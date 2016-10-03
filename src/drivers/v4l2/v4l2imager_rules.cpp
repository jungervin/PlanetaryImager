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


#include "v4l2imager_rules.h"
#include "v4l2imager.h"
#include <linux/videodev2.h>
using namespace std;

Drivers::V4L2::ControlFixes Drivers::V4L2::controlFixes(V4L2Imager *imager)
{
  return {
     // Exposure to combo in Microsoft Lifecam 3000
    [=](Imager::Control &s){
      if(s.id != V4L2_CID_EXPOSURE_ABSOLUTE || (imager->name() != "Microsoft\u00AE LifeCam HD-3000" && imager->name() != "Microsoft\u00AE LifeCam HD-5000"))
          return;
      s.type = Imager::Control::Combo;
      s.choices = {{"5", 5}, {"9", 9}, {"10", 10}, {"19", 19}, {"20", 20}, {"39", 39}, {"78", 78}, {"156", 156}, {"312", 312}, {"625", 625}, {"1250", 1250}, {"2500", 2500}, {"5000", 5000}, {"10000", 10000}};
    }
  };
}
