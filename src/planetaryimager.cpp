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
#include <QApplication>
#include "planetaryimager_mainwindow.h"
#include "commons/version.h"
#include <iostream>
#include <QDebug>
#include "drivers/supporteddrivers.h"
#include "commons/loghandler.h"
#include "commons/crashhandler.h"
#include "image_handlers/backend/local_saveimages.h"
#include "widgets/localfilesystembrowser.h"
#include "commons/commandline.h"
#include "network/server/networkserver.h"
#include "network/server/configurationforwarder.h"

using namespace std;


int main(int argc, char** argv)
{
    qRegisterMetaType<Frame::ptr>("Frame::ptr");
    qRegisterMetaType<Imager*>("Imager*");
    CrashHandler crash_handler({SIGSEGV, SIGABRT});
    cerr << "Starting PlanetaryImager - version " << PLANETARY_IMAGER_VERSION << " (" << HOST_PROCESSOR << ")" << endl;
    QApplication app(argc, argv);
    app.setApplicationName("PlanetaryImager");
    app.setApplicationDisplayName("Planetary Imager");
    app.setApplicationVersion(PLANETARY_IMAGER_VERSION);
    CommandLine commandLine(app);
    commandLine.backend().daemon("127.0.0.1").process();
    LogHandler log_handler{commandLine};
    
    auto configuration = make_shared<Configuration>();
    auto save_images = make_shared<LocalSaveImages>(configuration);
    auto drivers = make_shared<SupportedDrivers>(commandLine.driversDirectories());
    
    auto dispatcher = make_shared<NetworkDispatcher>();
    auto save_files_forwarder = make_shared<SaveFileForwarder>(save_images, dispatcher);
    auto configuration_forwarder = make_shared<ConfigurationForwarder>(configuration, dispatcher);
    auto frames_forwarder = make_shared<FramesForwarder>(dispatcher);
    
    PlanetaryImagerMainWindow mainWindow{
      drivers,
      save_images,
      configuration, make_shared<LocalFilesystemBrowser>(),
      commandLine.logfile()
    };
    auto imageHandlers = ImageHandler::ptr{new ImageHandlers{
      mainWindow.imageHandler(),
      frames_forwarder,
      save_images,
    }};
    auto server = make_shared<NetworkServer>(drivers, imageHandlers, dispatcher, save_files_forwarder, frames_forwarder );
    
    
    QMetaObject::invokeMethod(server.get(), "listen", Q_ARG(QString, commandLine.listenAddress()), Q_ARG(int, commandLine.port()));
    
    QObject::connect(server.get(), &NetworkServer::imagerConnected, &mainWindow, [&](Imager *imager) {
      mainWindow.setImager(imager);
    });
    QObject::connect(&mainWindow, &PlanetaryImagerMainWindow::quit, &app, &QApplication::quit);
    mainWindow.show();
    return app.exec();
}
