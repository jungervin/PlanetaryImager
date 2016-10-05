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

#include "autoguidermodel.h"
#include <QPointF>
#include <QtConcurrent/QtConcurrent>
#include <chrono>
#include "commons/utils.h"
using namespace std;
using namespace std::chrono_literals;

DPTR_IMPL(AutoguiderModel) {
  const Drivers::Autoguider::ptr guider;
  bool initialized = false;
  Status status = Idle;
  QPointF initial;
  void guide(const QPointF &coordinates);
  void learn(const QPointF &coordinates);
  void start_learning(Drivers::Autoguider::Direction direction);
};

AutoguiderModel::AutoguiderModel(const Drivers::Autoguider::ptr& guider) : dptr(guider)
{
}

AutoguiderModel::~AutoguiderModel()
{
}

void AutoguiderModel::learn()
{
  LOG_F_SCOPE;
  d->status = Learning;
    QtConcurrent::run([=]{
    d->start_learning(Drivers::Autoguider::North);
    d->start_learning(Drivers::Autoguider::South);
    d->start_learning(Drivers::Autoguider::East);
    d->start_learning(Drivers::Autoguider::West);
  });
}

void AutoguiderModel::Private::start_learning(Drivers::Autoguider::Direction direction)
{
  guider->guide(direction, 15ms);
}

void AutoguiderModel::Private::learn(const QPointF& coordinates)
{
}


void AutoguiderModel::new_coordinates(const QPointF& coordinates)
{
  if(d->status == Idle) {
    d->initial = coordinates;
    return;
  }
  if(d->status == Guiding) {
    d->guide(coordinates);
    return;
  }
  d->learn(coordinates);
}

void AutoguiderModel::Private::guide(const QPointF& coordinates)
{
}


bool AutoguiderModel::initialized() const
{
  return d->initialized;
}

AutoguiderModel::Status AutoguiderModel::status() const
{
  return d->status;
}

