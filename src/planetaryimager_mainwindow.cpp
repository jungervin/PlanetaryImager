/*
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "planetaryimager_mainwindow.h"
#include "drivers/driver.h"
#include "drivers/imager.h"
#include "ui_planetaryimager_mainwindow.h"
#include <functional>
#include "commons/utils.h"
#include "widgets/statusbarinfowidget.h"
#include <QLabel>
#include <QDoubleSpinBox>
#include <QThread>
#include <QFileDialog>
#include <QDateTime>
#include <QtConcurrent/QtConcurrent>
#include "commons/fps_counter.h"
#include "widgets/cameracontrolswidget.h"
#include "widgets/configurationdialog.h"
#include "commons/configuration.h"
#include <QMutex>
#include <QMessageBox>
#include "widgets/recordingpanel.h"
#include "widgets/camerainfowidget.h"
#include "widgets/histogramwidget.h"
#include "Qt/zoomableimage.h"
#include <QGridLayout>
#include <QToolBar>
#include <QWhatsThis>
#include "Qt/strings.h"
#include <Qt/functional.h>
#include <QGraphicsScene>
#include <QFileInfo>
#include <QDesktopServices>

#include "widgets/editroidialog.h"

#include "image_handlers/frontend/displayimage.h"
#include "image_handlers/saveimages.h"
#include "image_handlers/threadimagehandler.h"

#include "commons/messageslogger.h"
#include "commons/exposuretimer.h"


using namespace GuLinux;
using namespace std;
using namespace std::placeholders;


Q_DECLARE_METATYPE(cv::Mat)

DPTR_IMPL(PlanetaryImagerMainWindow) {
  Driver::ptr driver;
  SaveImages::ptr saveImages;
  Configuration::ptr configuration;
  FilesystemBrowser::ptr filesystemBrowser;
  unique_ptr<QThread> imagerThread;
  
  static PlanetaryImagerMainWindow *q;
  unique_ptr<Ui::PlanetaryImagerMainWindow> ui;
  Imager *imager = nullptr;
  void rescan_devices();
  void saveState();

  StatusBarInfoWidget *statusbar_info_widget;
  shared_ptr<DisplayImage> displayImage;
  Histogram::ptr histogram;
  CameraControlsWidget* cameraSettingsWidget = nullptr;
  CameraInfoWidget* cameraInfoWidget = nullptr;
  HistogramWidget *histogramWidget = nullptr;
  ConfigurationDialog *configurationDialog;
  EditROIDialog *editROIDialog;
  
  RecordingPanel* recording_panel;
  ExposureTimer exposure_timer;
  
  ImageHandler::ptr imageHandler;
  
  
  void connectCamera(const Driver::Camera::ptr &camera);
  void cameraDisconnected();
  void enableUIWidgets(bool cameraConnected);
  void init_devices_watcher();
  void editROI();
  ZoomableImage *image_widget;
  
  void onImagerInitialized(Imager *imager);
  
  enum SelectionMode { NoSelection, ROI, Guide } selection_mode = NoSelection;
};

PlanetaryImagerMainWindow *PlanetaryImagerMainWindow::Private::q = nullptr;

class CreateImagerWorker : public QObject {
  Q_OBJECT
public:
  typedef std::function<void(Imager *)> Slot;
  static void create(const Driver::Camera::ptr& camera, const ImageHandler::ptr& imageHandler, QThread* thread, QObject *context, Slot on_created);
private slots:
  void exec();
private:
  explicit CreateImagerWorker(const Driver::Camera::ptr& camera, const ImageHandler::ptr& imageHandler);
  Driver::Camera::ptr camera;
  ImageHandler::ptr imageHandler;
signals:
  void imager(Imager *imager);
};

CreateImagerWorker::CreateImagerWorker(const Driver::Camera::ptr& camera, const ImageHandler::ptr &imageHandler)
  : QObject(nullptr), camera{camera}, imageHandler{imageHandler}
{
}

void CreateImagerWorker::create(const Driver::Camera::ptr& camera, const ImageHandler::ptr& imageHandler, QThread* thread, QObject *context, Slot on_created)
{
  auto create_imager = new CreateImagerWorker(camera, imageHandler);
  create_imager->moveToThread(thread);
  connect(create_imager, &CreateImagerWorker::imager, context, on_created, Qt::QueuedConnection);
  QMetaObject::invokeMethod(create_imager, "exec", Qt::QueuedConnection);
}

void CreateImagerWorker::exec()
{
  try {
    auto imager = camera->imager(imageHandler);
    if(imager)
      emit this->imager(imager);
  } catch(const std::exception &e) {
    MessagesLogger::queue(MessagesLogger::Error, tr("Initialization Error"), tr("Error initializing imager %1: \n%2") % camera->name() % e.what());
  }
  deleteLater();
}


PlanetaryImagerMainWindow::~PlanetaryImagerMainWindow()
{
  LOG_F_SCOPE
  d->saveState();
  if(d->imager) {
      d->imager->destroy();
  }
  d->imagerThread->quit();
  d->imagerThread->wait();
  d->displayImage->quit();
  d->driver->aboutToQuit();
}

void PlanetaryImagerMainWindow::Private::saveState()
{
  configuration->set_dock_status(q->saveState());
  configuration->set_main_window_geometry(q->saveGeometry());
}


PlanetaryImagerMainWindow::PlanetaryImagerMainWindow(
      const Driver::ptr &driver,
      const SaveImages::ptr &save_images,
      const Configuration::ptr &configuration,
      const FilesystemBrowser::ptr &filesystemBrowser,
      const QString &logFilePath,
      QWidget* parent,
      Qt::WindowFlags flags
  )
: QMainWindow(parent, flags), dptr(driver, save_images, configuration, filesystemBrowser, make_unique<QThread>())
{
    Private::q = this;
    d->ui.reset(new Ui::PlanetaryImagerMainWindow);
    
    d->ui->setupUi(this);
    setWindowIcon(QIcon::fromTheme("planetary_imager"));
    d->ui->recording->setWidget(d->recording_panel = new RecordingPanel{d->configuration, filesystemBrowser});
    d->configurationDialog = new ConfigurationDialog(d->configuration, this);
    d->displayImage = make_shared<DisplayImage>(d->configuration);
    d->histogram = make_shared<Histogram>(d->configuration);
    d->ui->histogram->setWidget(d->histogramWidget = new HistogramWidget(d->histogram, d->configuration));
    d->ui->statusbar->addPermanentWidget(d->statusbar_info_widget = new StatusBarInfoWidget(), 1);

    d->ui->image->setLayout(new QGridLayout);
    d->ui->image->layout()->setMargin(0);
    d->ui->image->layout()->setSpacing(0);
    d->ui->image->layout()->addWidget(d->image_widget = new ZoomableImage(false));
#ifdef HAVE_QT5_OPENGL // TODO: make configuration item
    if(d->configuration->opengl())
      d->image_widget->setOpenGL();
#endif
    d->image_widget->scene()->setBackgroundBrush(QBrush{Qt::black, Qt::Dense4Pattern});
    connect(d->image_widget, &ZoomableImage::zoomLevelChanged, d->statusbar_info_widget, &StatusBarInfoWidget::zoom);
    d->statusbar_info_widget->zoom(d->image_widget->zoomLevel());
    for(auto item: d->image_widget->actions())
      d->ui->menuView->insertAction(d->ui->actionEdges_Detection, item);
    d->ui->menuView->insertSeparator(d->ui->actionEdges_Detection);
    
    d->image_widget->actions()[ZoomableImage::Actions::ZoomIn]->setShortcut({Qt::CTRL + Qt::Key_Plus});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomIn]->setIcon(QIcon{":/resources/zoom_in.png"});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomOut]->setShortcut({Qt::CTRL + Qt::Key_Minus});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomOut]->setIcon(QIcon{":/resources/zoom_out.png"});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomFit]->setShortcut({Qt::CTRL + Qt::Key_Space});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomFit]->setIcon(QIcon{":/resources/fit_window.png"});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomRealSize]->setShortcut({Qt::CTRL + Qt::Key_Backspace});
    d->image_widget->actions()[ZoomableImage::Actions::ZoomRealSize]->setIcon(QIcon{":/resources/real_size.png"});
    d->image_widget->toolbar()->setWindowTitle("Image Control");
    addToolBar(d->image_widget->toolbar());
    QToolBar *helpToolBar = new QToolBar;
    helpToolBar->setWindowTitle(tr("Help"));
    auto whatsThis = QWhatsThis::createAction();
    whatsThis->setIcon(QIcon{":/resources/help.png"});
    helpToolBar->addAction(whatsThis);
    addToolBar(helpToolBar);
    d->image_widget->toolbar()->setFloatable(true);
    d->image_widget->toolbar()->setMovable(true);
    
    restoreGeometry(d->configuration->main_window_geometry());
    restoreState(d->configuration->dock_status());
    connect(d->ui->actionAbout, &QAction::triggered, bind(&QMessageBox::about, this, tr("About"),
							  tr("%1 version %2.\nFast imaging capture software for planetary imaging").arg(qApp->applicationDisplayName())
							 .arg(qApp->applicationVersion())));
    connect(d->ui->actionAbout_Qt, &QAction::triggered, &QApplication::aboutQt);
    connect(d->ui->action_devices_rescan, &QAction::triggered, bind(&Private::rescan_devices, d.get()));
    connect(d->ui->actionShow_settings, &QAction::triggered, bind(&QDialog::show, d->configurationDialog));
    
    auto dockWidgetToggleVisibility = [=](QDockWidget *widget, bool visible){ widget->setVisible(visible); };
    auto dockWidgetVisibleCheck = [=](QAction *action, QDockWidget *widget) { action->setChecked(widget->isVisible()); };
    QList<QDockWidget*> dock_widgets;
    auto setupDockWidget = [&](QAction *action, QDockWidget *widget){
      dockWidgetVisibleCheck(action, widget);
      connect(action, &QAction::triggered, bind(dockWidgetToggleVisibility, widget, _1));
      connect(widget, &QDockWidget::visibilityChanged, bind(dockWidgetVisibleCheck, action, widget));
      connect(widget, &QDockWidget::dockLocationChanged, bind(&Private::saveState, d.get()));
      connect(widget, &QDockWidget::topLevelChanged, bind(&Private::saveState, d.get()));
      connect(widget, &QDockWidget::visibilityChanged, bind(&Private::saveState, d.get()));
      dock_widgets.push_back(widget);
    };

    connect(d->configurationDialog, &QDialog::accepted, this, bind(&DisplayImage::read_settings, d->displayImage), Qt::DirectConnection);
    connect(d->configurationDialog, &QDialog::accepted, this, bind(&Histogram::read_settings, d->histogram), Qt::DirectConnection);
    connect(d->recording_panel, &RecordingPanel::start, [=]{d->saveImages->startRecording(d->imager);});
    connect(d->recording_panel, &RecordingPanel::stop, bind(&SaveImages::endRecording, d->saveImages));
    connect(d->recording_panel, &RecordingPanel::setPaused, bind(&SaveImages::setPaused, d->saveImages, _1));
    
    connect(d->saveImages.get(), &SaveImages::recording, this, bind(&DisplayImage::setRecording, d->displayImage, true), Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::recording, this, bind(&Histogram::setRecording, d->histogram, true), Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::recording, d->recording_panel, bind(&RecordingPanel::recording, d->recording_panel, true, _1), Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::finished, d->recording_panel, bind(&RecordingPanel::recording, d->recording_panel, false, QString{}), Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::finished, this, [=]{
        QTimer::singleShot(5000,  bind(&DisplayImage::setRecording, d->displayImage, false));
        QTimer::singleShot(5000,  bind(&Histogram::setRecording, d->histogram, false));
    }, Qt::QueuedConnection);
    setupDockWidget(d->ui->actionChip_Info, d->ui->chipInfoWidget);
    setupDockWidget(d->ui->actionCamera_Settings, d->ui->camera_settings);
    setupDockWidget(d->ui->actionRecording, d->ui->recording);
    setupDockWidget(d->ui->actionHistogram, d->ui->histogram);
    if(! d->configuration->widgets_setup_first_run() ) {
      tabifyDockWidget(d->ui->chipInfoWidget, d->ui->camera_settings);
      tabifyDockWidget(d->ui->chipInfoWidget, d->ui->histogram);
      tabifyDockWidget(d->ui->chipInfoWidget, d->ui->recording);
      d->configuration->set_widgets_setup_first_run(true);
    }
    qDebug() << "file " << logFilePath << "exists: " << QFile::exists(logFilePath);
    d->ui->actionOpen_log_file_folder->setMenuRole(QAction::ApplicationSpecificRole);
    d->ui->actionOpen_log_file_folder->setVisible( ! logFilePath.isEmpty() && QFile::exists(logFilePath));
    connect(d->ui->actionOpen_log_file_folder, &QAction::triggered, this, [=]{
#ifdef Q_OS_MAC
        QStringList args;
        args << "-e";
        args << "tell application \"Finder\"";
        args << "-e";
        args << "activate";
        args << "-e";
        args << "select POSIX file \""+logFilePath+"\"";
        args << "-e";
        args << "end tell";
        QProcess::startDetached("osascript", args);
#else
          auto logFileDirectory = QFileInfo{logFilePath}.dir().path();
          if(!QDesktopServices::openUrl(logFileDirectory)) {
            MessagesLogger::instance()->queue(MessagesLogger::Warning, tr("Log file"), tr("Unable to open your log file. You can open it manually at this position: %1") % logFileDirectory);
          }
#endif
    });
    
    connect(d->ui->actionHide_all, &QAction::triggered, [=]{ for_each(begin(dock_widgets), end(dock_widgets), bind(&QWidget::hide, _1) ); });
    connect(d->ui->actionShow_all, &QAction::triggered, [=]{ for_each(begin(dock_widgets), end(dock_widgets), bind(&QWidget::show, _1) ); });
    connect(MessagesLogger::instance(), &MessagesLogger::message, this, bind(&PlanetaryImagerMainWindow::notify, this, _1, _2, _3, _4), Qt::QueuedConnection);
    d->rescan_devices();
    connect(d->displayImage.get(), &DisplayImage::gotImage, this, bind(&ZoomableImage::setImage, d->image_widget, _1), Qt::QueuedConnection);
    
    connect(d->ui->actionNight_Mode, &QAction::toggled, this, [=](bool checked) {
      qApp->setStyleSheet(checked ? R"_(
        * { background-color: rgb(40, 0, 0); color: rgb(220, 220, 220); }
      )_" : "");
    });

    
    connect(d->displayImage.get(), &DisplayImage::displayFPS, d->statusbar_info_widget, &StatusBarInfoWidget::displayFPS, Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::saveFPS, d->recording_panel, &RecordingPanel::saveFPS, Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::meanFPS, d->recording_panel, &RecordingPanel::meanFPS, Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::savedFrames, d->recording_panel, &RecordingPanel::saved, Qt::QueuedConnection);
    connect(d->saveImages.get(), &SaveImages::droppedFrames, d->recording_panel, &RecordingPanel::dropped, Qt::QueuedConnection);
    connect(d->ui->actionDisconnect, &QAction::triggered, this, [=]{
      QMetaObject::invokeMethod(d->imager, "destroy", Qt::QueuedConnection);
    });

    connect(d->ui->actionQuit, &QAction::triggered, this, &QWidget::close);
    connect(d->ui->actionQuit, &QAction::triggered, this, &PlanetaryImagerMainWindow::quit);
    d->enableUIWidgets(false);

    QtConcurrent::run(bind(&DisplayImage::create_qimages, d->displayImage));
    d->imagerThread->start();


    connect(d->ui->actionEdges_Detection, &QAction::toggled, [=](bool detect){
      d->displayImage->detectEdges(detect);
    });
    d->init_devices_watcher();
    connect(d->ui->actionClear_ROI, &QAction::triggered, [&] { d->imager->clearROI(); });
    connect(d->ui->actionSelect_ROI, &QAction::triggered, [&] { 
      d->selection_mode = Private::ROI;
      d->image_widget->startSelectionMode();
    });
    
    connect(d->ui->actionEdit_ROI, &QAction::triggered, this, bind(&Private::editROI, d.get()));
    QMap<Private::SelectionMode, function<void(const QRect &)>> handle_selection {
      {Private::NoSelection, [](const QRect&) {}},
      {Private::ROI, [&](const QRect &rect) { d->imager->setROI(rect.normalized()); }},
    };
    connect(d->image_widget, &ZoomableImage::selectedROI, [this, handle_selection](const QRectF &rect) {
      handle_selection[d->selection_mode](rect.toRect());
      d->image_widget->clearROI();
      d->selection_mode = Private::NoSelection;
    });
    connect(&d->exposure_timer, &ExposureTimer::progress, [=](double , double elapsed, double remaining){
      d->statusbar_info_widget->showMessage("Exposure: %1s, remaining: %2s"_q % QString::number(elapsed, 'f', 1) % QString::number(remaining, 'f', 1), 1000);
    });
    connect(&d->exposure_timer, &ExposureTimer::finished, [=]{ d->statusbar_info_widget->clearMessage(); });
    
    auto compositeImageHandler = ImageHandler::ptr{new ImageHandlers{d->displayImage, d->saveImages, d->histogram}};
    d->imageHandler = ImageHandler::ptr{new ThreadImageHandler{compositeImageHandler}};
    d->editROIDialog = new EditROIDialog(this);
    connect(d->editROIDialog, &EditROIDialog::roiSelected, this, [=](const QRect &roi){ d->imager->setROI(roi); });
    auto readTemperature = new QTimer{this};
    connect(readTemperature, &QTimer::timeout, this, [=] {
        if(d->imager)
            QMetaObject::invokeMethod(d->imager, "readTemperature", Qt::QueuedConnection);
    });
    readTemperature->start(2000);
}

void PlanetaryImagerMainWindow::closeEvent(QCloseEvent* event)
{
  QMainWindow::closeEvent(event);
  emit quit();
}

ImageHandler::ptr PlanetaryImagerMainWindow::imageHandler() const
{
  return d->imageHandler;
}


#include <iostream>

// TODO: extract to external class
void PlanetaryImagerMainWindow::Private::init_devices_watcher()
{
#ifdef Q_OS_LINUX
  auto notifyTimer = new QTimer(q);
  QString usbfsdir;
  for(auto path: QStringList{"/proc/bus/usb/devices", "/sys/bus/usb/devices"}) {
    if(QDir(path).exists())
      usbfsdir = path;
  }
  if(usbfsdir.isEmpty())
    return;
  connect(notifyTimer, &QTimer::timeout, [=]{
    static QStringList entries;
    auto current = QDir(usbfsdir).entryList();
    if(current != entries) {
      qDebug() << "usb devices changed";
      entries = current;
      rescan_devices();
    }
  });
  notifyTimer->start(1500);
#endif
}


void PlanetaryImagerMainWindow::Private::rescan_devices()
{
  ui->menu_device_load->clear();
  Thread::Run<Driver::Cameras>([=]{ return driver->cameras(); }, [=]( const Driver::Cameras &cameras){
    for(auto device: cameras) {
      auto message = tr("Found %1 devices").arg(cameras.size());
      qDebug() << message;
      statusbar_info_widget->showMessage(message, 10'000);
      QAction *action = ui->menu_device_load->addAction(device->name());
      QObject::connect(action, &QAction::triggered, bind(&Private::connectCamera, this, device));
    }
  });
}

void PlanetaryImagerMainWindow::connectCamera(const Driver::Camera::ptr& camera)
{
  d->connectCamera(camera);
}


void PlanetaryImagerMainWindow::Private::connectCamera(const Driver::Camera::ptr& camera)
{
    if(imager)
        imager->destroy();
  CreateImagerWorker::create(camera, imageHandler, imagerThread.get(), q, bind(&Private::onImagerInitialized, this, _1) );
}

void PlanetaryImagerMainWindow::setImager(Imager* imager)
{
  d->onImagerInitialized(imager);
}


void PlanetaryImagerMainWindow::Private::onImagerInitialized(Imager * imager)
{  
    if(!imager) {
      return;
    }
    cameraDisconnected();
    this->imager = imager;
    exposure_timer.set_imager(imager);
    imager->startLive();
    statusbar_info_widget->deviceConnected(imager->name());
    connect(imager, &Imager::disconnected, q, bind(&Private::cameraDisconnected, this), Qt::QueuedConnection);
    connect(imager, &Imager::fps, statusbar_info_widget, &StatusBarInfoWidget::captureFPS, Qt::QueuedConnection);
    connect(imager, &Imager::temperature, statusbar_info_widget, bind(&StatusBarInfoWidget::temperature, statusbar_info_widget, _1, false), Qt::QueuedConnection);

    ui->settings_container->setWidget(cameraSettingsWidget = new CameraControlsWidget(imager, configuration, filesystemBrowser));
    ui->chipInfoWidget->setWidget(cameraInfoWidget = new CameraInfoWidget(imager));
    enableUIWidgets(true);
    ui->actionSelect_ROI->setEnabled(imager->supports(Imager::ROI));
    ui->actionEdit_ROI->setEnabled(imager->supports(Imager::ROI));
    ui->actionClear_ROI->setEnabled(imager->supports(Imager::ROI));
}


void PlanetaryImagerMainWindow::Private::cameraDisconnected()
{
  imager = nullptr;
  qDebug() << "camera disconnected";
  enableUIWidgets(false);
  ui->actionSelect_ROI->setEnabled(false);
  ui->actionEdit_ROI->setEnabled(false);
  ui->actionClear_ROI->setEnabled(false);
  
  delete cameraSettingsWidget;
  cameraSettingsWidget = nullptr;
  delete cameraInfoWidget;
  cameraInfoWidget = nullptr;
  image_widget->setImage({});
  statusbar_info_widget->captureFPS(0);
  statusbar_info_widget->temperature(0, true);
}

void PlanetaryImagerMainWindow::Private::enableUIWidgets(bool cameraConnected)
{
  ui->actionDisconnect->setEnabled(cameraConnected);
  ui->recording->setEnabled(cameraConnected);
  ui->chipInfoWidget->setEnabled(cameraConnected);
  ui->camera_settings->setEnabled(cameraConnected);
}

void PlanetaryImagerMainWindow::notify(const QDateTime &when, MessagesLogger::Type notification_type, const QString& title, const QString& message)
{
  static QHash<MessagesLogger::Type, function<void(const QString&, const QString&)>> types_map {
    {MessagesLogger::Warning, [](const QString &title, const QString &message) { QMessageBox::warning(nullptr, title, message); }},
    {MessagesLogger::Error, [](const QString &title, const QString &message) { QMessageBox::critical(nullptr, title, message); }},
    {MessagesLogger::Info, [](const QString &title, const QString &message) { QMessageBox::information(nullptr, title, message); }},
  };
  types_map[notification_type](title, message);
}


void PlanetaryImagerMainWindow::Private::editROI()
{
  auto resolution = imager->properties().resolution();
  editROIDialog->setResolution(resolution);
  editROIDialog->show();
  //TODO check for resolution not existing
  // TODO check for current ROI
}



#include "planetaryimager_mainwindow.moc"
