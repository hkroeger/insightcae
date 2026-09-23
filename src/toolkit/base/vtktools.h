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
 * This header used to bundle several unrelated VTK helper groups. It has
 * been split into base/vtkcoords.h, base/surfacevectorsfilter.h,
 * base/vtklegacymodel.h, base/vtknormals.h, base/vtkgridconversion.h and
 * base/vtkobjectsize.h. Only the groups with broad/unclear external usage
 * are still re-included here for backward compatibility; consumers of the
 * narrowly-used groups now include the specific new header directly.
 *
 */

#ifndef INSIGHT_VTK_H
#define INSIGHT_VTK_H

#include "base/vtknormals.h"
#include "base/vtkobjectsize.h"

#endif // INSIGHT_VTK_H
