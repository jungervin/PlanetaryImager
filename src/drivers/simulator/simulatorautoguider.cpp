/*
 * GuLinux Planetary Imager - https://github.com/GuLinux/PlanetaryImager
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
#include "commons/utils.h"
#include "c++/stlutils.h"
#include "simulatorautoguider.h"
#include <QThread>
#include <QFile>
#include <QPointF>
using namespace std;

class AutoguiderWorker;

DPTR_IMPL(SimulatorAutoguider) {
  shared_ptr<AutoguiderWorker> worker;
};


class AutoguiderWorker : public ImagerThread::Worker {
public:
  AutoguiderWorker();
  static const cv::Size image_size;
  Frame::ptr shoot() override;
  void setROI(const QRect &roi);
private:
  QPointF coordinates;
  cv::Mat image;
  LOG_C_SCOPE(AutoguiderWorker);
};

const cv::Size AutoguiderWorker::image_size = cv::Size{1280, 1024};

AutoguiderWorker::AutoguiderWorker()
{
  QFile file(":/simulator/jupiter_gulinux.jpg");
  file.open(QIODevice::ReadOnly);
  QByteArray file_data = file.readAll();
  image = cv::imdecode(cv::InputArray{file_data.data(), file_data.size()}, CV_LOAD_IMAGE_GRAYSCALE);
  coordinates = {
    static_cast<qreal>(image_size.width)/2. - image.cols/2,
    static_cast<qreal>(image_size.height)/2. - image.rows/2
  };
}

void AutoguiderWorker::setROI(const QRect& roi)
{
}

Frame::ptr AutoguiderWorker::shoot()
{
  GuLinux::Scope sleep{[]{QThread::currentThread()->msleep(10); }};
  cv::Mat result{image_size, CV_8UC1, cv::Scalar{0}};
  cv::Mat roi(result, cv::Rect{static_cast<int>(coordinates.x()), static_cast<int>(coordinates.y()), image.cols, image.rows});
  image.copyTo(roi);
  return make_shared<Frame>(Frame::Mono, result);
}



SimulatorAutoguider::SimulatorAutoguider(const ImageHandler::ptr& handler) : Imager{handler}
{
}

QString SimulatorAutoguider::name() const
{
  return "Autoguider Simulator";
}

void SimulatorAutoguider::clearROI()
{
}

Imager::Controls SimulatorAutoguider::controls() const
{
  return {};
}

Imager::Properties SimulatorAutoguider::properties() const
{
  return {};
}

void SimulatorAutoguider::setControl(const Imager::Control& control)
{
}

void SimulatorAutoguider::setROI(const QRect&)
{
}

void SimulatorAutoguider::startLive()
{
  restart([]{ return make_shared<AutoguiderWorker>(); });
}

bool SimulatorAutoguider::supportsROI() const
{
  return false;
}




