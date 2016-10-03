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
#include <opencv2/opencv.hpp>

#include <opencv2/tracking.hpp>

using namespace std;

DPTR_IMPL(AutoGuider) {
  QGraphicsScene* scene;
  Configuration &configuration;
    
  enum TrackMode { TrackNone, TrackInit, Track } track_mode = TrackNone;
  cv::Rect2d track_roi;
  cv::Ptr<cv::Tracker> tracker;
  QGraphicsRectItem *rectItem;
};

AutoGuider::AutoGuider(QGraphicsScene* scene, Configuration &configuration) : dptr(scene, configuration)
{
}

AutoGuider::~AutoGuider()
{
}


void AutoGuider::handle(const Frame::ptr& frame)
{
    if(d->track_mode == Private::TrackInit) {
      bool initialized = d->tracker = cv::Tracker::create("KCF");
      qDebug() << "Tracker KCF created: " << initialized;
      //bool initialized = d->tracker = cv::Tracker::create("TLD");
      if(initialized) {
        initialized &= d->tracker->init(frame->mat(), d->track_roi);
        qDebug() << "Tracker KCF initialized: " << initialized;
      }
      d->track_mode = initialized ? Private::Track : Private::TrackNone;
    } else if(d->track_mode == Private::Track) {
      d->tracker->update(frame->mat(), d->track_roi);
      d->rectItem->setRect(d->track_roi.x, d->track_roi.y, d->track_roi.width, d->track_roi.height);
      QMetaObject::invokeMethod(d->scene, "update", Qt::QueuedConnection);
      //cv::rectangle( *cv_image, d->track_roi, cv::Scalar( 255, 0, 0 ), 2, 1 );
    }

}

void AutoGuider::track(const QRect& rect)
{
  d->track_roi = {rect.x(), rect.y(), rect.width(), rect.height()};
  d->rectItem = d->scene->addRect(rect, {Qt::red}, {QColor{0, 250, 250, 10}});
  d->rectItem->setZValue(2);
  d->rectItem->setFlag(QGraphicsItem::ItemIsMovable, true);
  d->track_mode = Private::TrackInit;
}

