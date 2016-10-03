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

#ifndef SIMULATORAUTOGUIDER_H
#define SIMULATORAUTOGUIDER_H
#include "drivers/imager.h"
#include "c++/dptr.h"

class SimulatorAutoguider : public Imager
{
public:
  SimulatorAutoguider(const ImageHandler::ptr &handler);
  void startLive() override;
  Imager::Properties properties() const override;
  QString name() const override;
  Controls controls() const override;
  void setControl(const Imager::Control & control) override;
  void clearROI() override;
  bool supportsROI() const override;
  void setROI(const QRect & ) override;
private:
  DPTR
};

#endif // SIMULATORAUTOGUIDER_H
