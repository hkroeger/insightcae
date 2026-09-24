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
 */


#include "vtkobjectsize.h"

#include "base/cppextensions.h"

#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointSet.h"
#include "vtkDataObject.h"

namespace insight
{

size_t computeObjectSize(vtkSmartPointer<vtkPolyData> pd)
{
    return pd->GetActualMemorySize()*1024;
}


size_t computeObjectSize(vtkSmartPointer<vtkUnstructuredGrid> pd)
{
    return pd->GetActualMemorySize()*1024;
}


size_t computeObjectSize(vtkSmartPointer<vtkPointSet> pd)
{
    return pd->GetActualMemorySize()*1024;
}

size_t computeObjectSize(vtkSmartPointer<vtkDataObject> pd)
{
    return pd->GetActualMemorySize()*1024;
}

}

namespace std {

std::size_t hash<vtkSmartPointer<vtkDataObject> >::operator()
    (const vtkSmartPointer<vtkDataObject>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<size_t>()(v->GetActualMemorySize()));
//    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
//    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfCells()));
//    auto np=v->GetNumberOfPoints();
//    auto step=std::max<vtkIdType>(1, np/4);
//    for (vtkIdType i=0; i<np; i+=step)
//    {
//      double x[3];
//      v->GetPoint(i, x);
//      for (int j=0; j<3; ++j)
//        std::hash_combine(h, std::hash<double>()(x[j]));
//    }
    return h;
}

std::size_t hash<vtkSmartPointer<vtkPolyData> >::operator()
    (const vtkSmartPointer<vtkPolyData>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfCells()));
    auto np=v->GetNumberOfPoints();
    auto step=std::max<vtkIdType>(1, np/4);
    for (vtkIdType i=0; i<np; i+=step)
    {
      double x[3];
      v->GetPoint(i, x);
      for (int j=0; j<3; ++j)
        std::hash_combine(h, std::hash<double>()(x[j]));
    }
    return h;
}


std::size_t hash<vtkSmartPointer<vtkUnstructuredGrid> >::operator()
    (const vtkSmartPointer<vtkUnstructuredGrid>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfCells()));
    auto np=v->GetNumberOfPoints();
    auto step=std::max<vtkIdType>(1, np/4);
    for (vtkIdType i=0; i<np; i+=step)
    {
        double x[3];
        v->GetPoint(i, x);
        for (int j=0; j<3; ++j)
            std::hash_combine(h, std::hash<double>()(x[j]));
    }
    return h;
}

std::size_t hash<vtkSmartPointer<vtkPointSet> >::operator()
    (const vtkSmartPointer<vtkPointSet>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
    auto np=v->GetNumberOfPoints();
    auto step=std::max<vtkIdType>(1, np/4);
    for (vtkIdType i=0; i<np; i+=step)
    {
      double x[3];
      v->GetPoint(i, x);
      for (int j=0; j<3; ++j)
        std::hash_combine(h, std::hash<double>()(x[j]));
    }
    return h;
}

}
