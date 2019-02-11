/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "units.h"
#include "base/linearalgebra.h"

namespace SI
{

double ton=1e3;
double kg=1.;
double gram=1e-3;
double lb=0.45359237;

double microns=1e-6;
double mm=1e-3;
double cm=0.01;
double m=1.;
double km=1e3;
double in=0.0254;


double rad=1.;
double deg=M_PI/180.;

double sec = 1.;
double min = 60.;
double hr = 60.*min;
double day = 24.*hr;


double mps=1.;
double kmh=3.6;
double kt=0.51444;

}
