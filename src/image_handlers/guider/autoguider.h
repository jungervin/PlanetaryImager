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

#ifndef AUTOGUIDER_H
#define AUTOGUIDER_H

#include "image_handlers/imagehandler.h"
#include "drivers/autoguider.h"

class QGraphicsScene;
class Configuration;
class QRect;
class AutoGuider : public ImageHandler
{
public:
  AutoGuider(QGraphicsScene *scene, Configuration &configuration);
  virtual ~AutoGuider();
  typedef std::shared_ptr<AutoGuider> ptr;
    void handle(const Frame::ptr &frame);
    void track(const QRect &rect);
    void setGuider(const Drivers::Autoguider::ptr &autoguider);
private:
  DPTR
};

#endif // AUTOGUIDER_H
