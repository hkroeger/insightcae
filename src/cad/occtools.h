/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  hannes <email>
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
 * This header used to bundle ~15 independent OCC helper functions/classes.
 * It has been split into occconvert.h, occbndbox.h, occtransform.h,
 * occshapeutils.h, occedgetools.h, occfacetools.h and occtriangulation.h,
 * all still re-included here for full backward compatibility. occtools.h
 * is circularly re-included by occinclude.h (occinclude.h includes
 * occtools.h, and vice versa), so its real fan-out can't be safely bounded
 * — no group has been narrowed out of this umbrella.
 *
 */

#ifndef INSIGHT_OCCTOOLS_H
#define INSIGHT_OCCTOOLS_H

#include "occconvert.h"
#include "occbndbox.h"
#include "occtransform.h"
#include "occshapeutils.h"
#include "occedgetools.h"
#include "occfacetools.h"
#include "occtriangulation.h"

#endif // INSIGHT_OCCTOOLS_H
