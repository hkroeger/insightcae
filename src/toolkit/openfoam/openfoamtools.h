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
 *
 * This header used to bundle ~60 declarations across ~9 unrelated groups.
 * It has been split into oftimedirectories.h, ofmeshcopy.h, ofmeshquality.h,
 * ofcsvreaders.h, ofdictreaders.h, ofpatchutils.h, ofparalleltools.h,
 * emesh.h, hydrostaticpressure.h, ofmiscutils.h and
 * homogeneousaveragedprofile.h, all still re-included here for full
 * backward compatibility (no group has been narrowed out of this umbrella).
 * The two includes this header used to carry unconditionally but which had
 * zero real use in the declarations here (openfoam/sampling.h,
 * boost/ptr_container/ptr_vector.hpp) were dropped — they are only needed
 * by homogeneousaveragedprofile.cpp's implementation, which includes them
 * directly.
 *
 */

#ifndef OPENFOAMTOOLS_H
#define OPENFOAMTOOLS_H

#include "openfoam/oftimedirectories.h"
#include "openfoam/ofmeshcopy.h"
#include "openfoam/ofmeshquality.h"
#include "openfoam/ofcsvreaders.h"
#include "openfoam/ofdictreaders.h"
#include "openfoam/ofpatchutils.h"
#include "openfoam/ofparalleltools.h"
#include "openfoam/emesh.h"
#include "openfoam/hydrostaticpressure.h"
#include "openfoam/ofmiscutils.h"
#include "openfoam/homogeneousaveragedprofile.h"

#endif // OPENFOAMTOOLS_H
