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

#ifndef UNITS_H
#define UNITS_H

namespace SI
{

// the units below represent the conversion factor from the unit under consideration into the corresponding SI standard unit
// e.g.:   <length in meters> = <length in millimeters> * SI:mm;  (i.e. SI:mm == 1e-3)

// mass

extern double ton;
extern double kg;
extern double gram;
extern double lb;

// length
extern double microns;
extern double mm;
extern double cm;
extern double m;
extern double km;
extern double in;

// time
extern double sec;
extern double min;
extern double hr;
extern double day;

// angle
extern double rad;
extern double deg;

// velocity
extern double mps;
extern double kmh;
extern double kt;

}

#endif // UNITS_H
