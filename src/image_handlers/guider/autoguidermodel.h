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

#ifndef AUTOGUIDERMODEL_H
#define AUTOGUIDERMODEL_H
#include "drivers/autoguider.h"
#include "c++/dptr.h"

class QPointF;
class AutoguiderModel
{
public:
  typedef std::shared_ptr<AutoguiderModel> ptr;
  AutoguiderModel(const Drivers::Autoguider::ptr &guider);
  ~AutoguiderModel();
  bool initialized() const;
  void learn();
  void new_coordinates(const QPointF &coordinates);
private:
  DPTR
};

#endif // AUTOGUIDERMODEL_H
