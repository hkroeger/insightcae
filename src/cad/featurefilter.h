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

#ifndef INSIGHT_CAD_FEATUREFILTER_H
#define INSIGHT_CAD_FEATUREFILTER_H

#include <set>
#include <map>
#include <vector>
// #include <iostream>

#include "base/boost_include.h"
#include "base/linearalgebra.h"
#include "feature.h"

// quantity computers
#include "constantquantity.h"
#include "quantityfunctions.h"
#include "relationfilters.h"
#include "cylradius.h"
#include "cylaxis.h"
#include "vertexlocation.h"
#include "edgecog.h"
#include "edgeend.h"
#include "edgestart.h"
#include "edgemidtangent.h"
#include "edgeradiallen.h"
#include "facearea.h"
#include "facecog.h"
#include "facenormalvector.h"
#include "edgelen.h"
#include "distance.h"
#include "solidcog.h"
#include "solidvolume.h"

// filters
#include "in.h"
#include "everything.h"
#include "approximatelyequal.h"
#include "booleanfilters.h"
#include "maximal.h"
#include "minimal.h"
#include "relationfilters.h"

#include "coincident.h"
#include "identical.h"
#include "ispartofsolid.h"

#include "boundaryedge.h"
#include "edgetopology.h"
#include "cylfaceorientation.h"
#include "coincidentprojectededge.h"
#include "secant.h"

#include "faceadjacenttoedges.h"
#include "faceadjacenttofaces.h"
#include "facetopology.h"
#include "cylfaceorientation.h"
#include "boundaryofface.h"

namespace insight
{
namespace cad
{


FilterPtr parseVertexFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );
FilterPtr parseEdgeFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );
FilterPtr parseFaceFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );
FilterPtr parseSolidFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );

}
}

#endif
