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
 * NOTE: base/cacheableentity.h's generic Cached<T,...> template calls
 * computeObjectSize(result) and instantiates std::hash<T>() for whatever
 * VTK-smart-pointer T a caller uses, without naming these symbols directly.
 * Keep this header included unconditionally by base/vtktools.h (do not
 * narrow it out of the umbrella).
 *
 */


#ifndef INSIGHT_VTKOBJECTSIZE_H
#define INSIGHT_VTKOBJECTSIZE_H

#include <cstddef>

#include "vtkSmartPointer.h"

class vtkPolyData;
class vtkUnstructuredGrid;
class vtkPointSet;
class vtkDataObject;

namespace insight {

size_t computeObjectSize(vtkSmartPointer<vtkPolyData> pd);
size_t computeObjectSize(vtkSmartPointer<vtkUnstructuredGrid> pd);
size_t computeObjectSize(vtkSmartPointer<vtkPointSet> pd);
size_t computeObjectSize(vtkSmartPointer<vtkDataObject> pd);

}

namespace std {

template<> struct hash<vtkSmartPointer<vtkDataObject> >
{
    std::size_t operator()(const vtkSmartPointer<vtkDataObject>& v) const;
};

template<> struct hash<vtkSmartPointer<vtkPolyData> >
{
    std::size_t operator()(const vtkSmartPointer<vtkPolyData>& v) const;
};

template<> struct hash<vtkSmartPointer<vtkUnstructuredGrid> >
{
    std::size_t operator()(const vtkSmartPointer<vtkUnstructuredGrid>& v) const;
};

template<> struct hash<vtkSmartPointer<vtkPointSet> >
{
    std::size_t operator()(const vtkSmartPointer<vtkPointSet>& v) const;
};

}

#endif // INSIGHT_VTKOBJECTSIZE_H
