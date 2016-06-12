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

#include "viewstate.h"

#include <iostream>

ViewState::ViewState(int shad)
: shading(shad),
  visible(true),
  r(0.5),
  g(0.5),
  b(0.5)
{
  randomizeColor();
}

void ViewState::randomizeColor()
{
  r=0.5+0.5*( double(rand()) / double(RAND_MAX) );
  g=0.5+0.5*( double(rand()) / double(RAND_MAX) );
  b=0.5+0.5*( double(rand()) / double(RAND_MAX) );
}
