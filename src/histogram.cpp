/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  <copyright holder> <email>
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

#include "histogram.h"
#include <functional>
#include <opencv2/opencv.hpp>

#define cimg_display 0
#define cimg_plugin "CImg/plugins/cvMat.h"
#include <CImg.h>
using namespace cimg_library;

using namespace std;

class Histogram::Private {
public:
  Private(Histogram *q);
  QElapsedTimer last;
  size_t bins_size;
private:
  Histogram *q;
};

Histogram::Private::Private(Histogram* q) : q{q}
{
}


Histogram::~Histogram()
{
}

Histogram::Histogram(QObject* parent) : QObject(parent), dptr(this)
{
  d->last.start();
}

void Histogram::handle(const cv::Mat& imageData)
{
  if(d->last.elapsed() < 1000)
    return;

  CImg<uint32_t> image(imageData);
  map<int, uint32_t> depths {
    {CV_8U, numeric_limits<uint8_t>::max()},
    {CV_8S, numeric_limits<int8_t>::max()},
    {CV_16U, numeric_limits<uint16_t>::max()},
    {CV_16S, numeric_limits<int16_t>::max()},
  };
  image.histogram(d->bins_size, 0, depths[imageData.depth()]);
  vector<uint32_t> hist(image.size());
  move(image.begin(), image.end(), hist.begin());
  
  emit histogram(hist);
  d->last.restart();
}


void Histogram::set_bins(size_t bins_size)
{
  d->bins_size = bins_size;
}

#include "histogram.moc"