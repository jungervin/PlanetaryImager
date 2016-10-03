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

#ifndef DRIVERS_AUTOGUIDER_H
#define DRIVERS_AUTOGUIDER_H
#include <memory>
#include <functional>
#include <chrono>
namespace Drivers {

class Autoguider
{
public:
  typedef std::shared_ptr<Autoguider> ptr;
  enum Direction {North, South, East, West };
  virtual bool guide(Direction direction, const std::chrono::milliseconds &duration) = 0;
  typedef std::function<bool(Direction, const std::chrono::milliseconds &)> Guide;
};
}

#endif // DRIVERS_AUTOGUIDER_H
