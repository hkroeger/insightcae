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

#ifndef INSIGHT_GEOTEST_H
#define INSIGHT_GEOTEST_H

#include "occinclude.h"

namespace insight 
{
namespace cad 
{

  

//BoundingBox
bool isPartOf(const Bnd_Box& big, const Bnd_Box&  small, double tolerance=0.001);
//EdgeTest
bool isPartOf(const TopoDS_Edge& big, const TopoDS_Vertex& p, double tolerance=0.001);
bool isPartOf(const TopoDS_Edge& big, const TopoDS_Edge& e, double tolerance=0.001, int nSamples=10);
//FaceTest
bool isPartOf(const TopoDS_Face& big, const TopoDS_Vertex& p, double tolerance=0.001);
bool isPartOf(const TopoDS_Face& big, const TopoDS_Edge& e, double tolerance=0.001, int nSamples=10);
bool isPartOf(const TopoDS_Face& big, const TopoDS_Face& small, double tolerance=0.001, int nSamples=10);
//ShellTest
//bool isPartOf(const TopoDS_Shell& big, const TopoDS_Edge& e, double tolerance=0.001, int nSamples=10);
bool isPartOf(const TopoDS_Shell& big, const TopoDS_Face& small, double tolerance=0.001, int nSamples=10);
//SolidTest
bool isPartOf(const TopoDS_Solid& big, const TopoDS_Shape& small, double tolerance=.0001, int nSamples=10);
//Allgemein
bool isPartOf(const TopoDS_Shape& big, const TopoDS_Shape& small, double tolerance=0.001, int nSamples=10);

  
bool isEqual
(
    const TopoDS_Edge& e1,
    const TopoDS_Edge& e2,
    double tol=1e-5
);

bool isEqual
(
    const TopoDS_Face& f1,
    const TopoDS_Face& f2,
    double etol=1e-3
);

bool isEmptyShape(const TopoDS_Shape& s);

bool isGeometricallyIdentical
(
    const TopoDS_Shape& s1,
    const TopoDS_Shape& s2
);

}  
}

#endif // INSIGHT_GEOTEST_H
