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

#include "autoguider.h"
#include <QGraphicsScene>
#include "commons/configuration.h"
#include <QDebug>
#include <QGraphicsRectItem>
#include <QElapsedTimer>
#include <opencv2/opencv.hpp>
#include "commons/opencv_utils.h"

#include <opencv2/tracking.hpp>

using namespace std;
class AutoGuiderRect : public QObject {
  Q_OBJECT
public:
  typedef shared_ptr<AutoGuiderRect> ptr;
  AutoGuiderRect(QGraphicsScene *scene, const QRectF &rect);
  ~AutoGuiderRect();
public slots:
  void update(const QRectF &rect);
private:
  QGraphicsScene *scene;
  QGraphicsRectItem *rectItem = nullptr;
};

DPTR_IMPL(AutoGuider) {
  QGraphicsScene* scene;
  Configuration &configuration;
  AutoGuiderRect::ptr autoguider_rect;
  enum TrackMode { TrackNone, TrackInit, Track } track_mode = TrackNone;
  cv::Rect2d track_roi;
  cv::Ptr<cv::Tracker> tracker;
  QElapsedTimer elapsed;
};

AutoGuider::AutoGuider(QGraphicsScene* scene, Configuration &configuration) : dptr(scene, configuration)
{
}

AutoGuider::~AutoGuider()
{
}


AutoGuiderRect::AutoGuiderRect(QGraphicsScene* scene, const QRectF& rect) : scene{scene}, rectItem{scene->addRect(rect, {Qt::red}, {QColor{0, 250, 250, 10}})}
{
  rectItem->setZValue(2);
}

void AutoGuiderRect::update(const QRectF& rect)
{
  rectItem->setRect(rect);
}

AutoGuiderRect::~AutoGuiderRect()
{
  scene->removeItem(rectItem);
  delete rectItem;
}


void AutoGuider::handle(const Frame::ptr& frame)
{
  if(d->track_mode == Private::TrackNone || (d->track_mode == Private::Track && d->elapsed.elapsed() < 100) )
    return;
  cv::Mat image = frame->mat();
  if(image.channels() == 1) {
    cv::cvtColor(image, image, CV_GRAY2BGR);
  }
  d->elapsed.restart();
  if(d->track_mode == Private::TrackInit) {
    bool initialized = d->tracker = cv::Tracker::create("KCF");
    qDebug() << "Tracker KCF created: " << initialized;
    //bool initialized = d->tracker = cv::Tracker::create("TLD");
    if(initialized) {
      initialized &= d->tracker->init(image, d->track_roi);
      qDebug() << "Tracker KCF initialized: " << initialized;
    }
    d->track_mode = initialized ? Private::Track : Private::TrackNone;
  } else if(d->track_mode == Private::Track) {
    d->tracker->update(image, d->track_roi);
    QMetaObject::invokeMethod(d->autoguider_rect.get(), "update", Q_ARG(QRectF, Utils::cv::cv2qrectf(d->track_roi)));
  }
}

void AutoGuider::track(const QRect& rect)
{
  d->track_roi = Utils::cv::qrect2cv(rect);
  d->autoguider_rect = make_shared<AutoGuiderRect>(d->scene, rect);
  d->track_mode = Private::TrackInit;
}

void AutoGuider::setGuider(const Drivers::Autoguider::ptr& autoguider)
{
  qDebug() << "Setting autoguider: " << autoguider.operator bool();
}


#include "autoguider.moc"
