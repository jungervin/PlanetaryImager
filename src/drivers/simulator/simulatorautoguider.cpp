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

enum AutoguiderControlType {
  HorizontalDrift = 0,
  VerticalDrift = 1,
};
typedef QHash<AutoguiderControlType, Imager::Control> AutoguiderControls;
DPTR_IMPL(SimulatorAutoguider) {
  shared_ptr<AutoguiderWorker> worker;
  AutoguiderControls controls;
};


class AutoguiderWorker : public ImagerThread::Worker {
public:
  AutoguiderWorker(AutoguiderControls &controls);
  static const cv::Size image_size;
  Frame::ptr shoot() override;
  void setROI(const QRect &);
private:
  AutoguiderControls &controls;
  QPointF coordinates;
  cv::Mat image;
  LOG_C_SCOPE(AutoguiderWorker);
};

const cv::Size AutoguiderWorker::image_size = cv::Size{1280, 1024};

AutoguiderWorker::AutoguiderWorker(AutoguiderControls &controls) : controls{controls}
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

void AutoguiderWorker::setROI(const QRect&)
{
}

Frame::ptr AutoguiderWorker::shoot()
{
  GuLinux::Scope sleep{[]{QThread::currentThread()->msleep(10); }};
  cv::Mat result{image_size, CV_8UC1, cv::Scalar{0}};
  auto roi_size = cv::Rect{static_cast<int>(coordinates.x()), static_cast<int>(coordinates.y()), image.cols, image.rows} & cv::Rect{0, 0, result.cols, result.rows};
  cv::Mat roi(result, roi_size);
  image.copyTo(roi);
  coordinates += QPointF{controls[HorizontalDrift].value, controls[VerticalDrift].value};
  return make_shared<Frame>(Frame::Mono, result);
}



SimulatorAutoguider::SimulatorAutoguider(const ImageHandler::ptr& handler) : Imager{handler}, dptr()
{
  d->controls = {
    {HorizontalDrift, {HorizontalDrift, "HorizontalDrift", -2, 2, 0.01, 0, 0, Control::Number}},
    {VerticalDrift, {VerticalDrift, "VerticalDrift", -2, 2, 0.01, 0, 0, Control::Number}},
  };
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
  return d->controls.values();
}

Imager::Properties SimulatorAutoguider::properties() const
{
  return {};
}

void SimulatorAutoguider::setControl(const Imager::Control& control)
{
  push_job_on_thread([this, control]{
    d->controls[static_cast<AutoguiderControlType>(control.id)] = control;
    emit changed(control);
  });
}

void SimulatorAutoguider::setROI(const QRect&)
{
}

void SimulatorAutoguider::startLive()
{
  restart([this]{ return make_shared<AutoguiderWorker>(d->controls); });
}

bool SimulatorAutoguider::supportsROI() const
{
  return false;
}




