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
#include "quantitycomputers/constantquantity.h"
#include "quantitycomputers/quantityfunctions.h"
#include "featurefilters/relationfilters.h"
#include "quantitycomputers/cylradius.h"
#include "quantitycomputers/cylaxis.h"
#include "quantitycomputers/vertexlocation.h"
#include "quantitycomputers/edgecog.h"
#include "quantitycomputers/edgeend.h"
#include "quantitycomputers/edgestart.h"
#include "quantitycomputers/edgemidtangent.h"
#include "quantitycomputers/edgeradiallen.h"
#include "quantitycomputers/facearea.h"
#include "quantitycomputers/facecog.h"
#include "quantitycomputers/facenormalvector.h"
#include "quantitycomputers/edgelen.h"
#include "quantitycomputers/distance.h"
#include "quantitycomputers/solidcog.h"
#include "quantitycomputers/solidvolume.h"

// filters
#include "featurefilters/in.h"
#include "featurefilters/everything.h"
#include "featurefilters/approximatelyequal.h"
#include "featurefilters/booleanfilters.h"
#include "featurefilters/maximal.h"
#include "featurefilters/minimal.h"
#include "featurefilters/relationfilters.h"

#include "featurefilters/connected.h"
#include "featurefilters/coincident.h"
#include "featurefilters/identical.h"
#include "featurefilters/ispartofsolid.h"

#include "featurefilters/boundaryedge.h"
#include "featurefilters/edgetopology.h"
#include "featurefilters/cylfaceorientation.h"
#include "featurefilters/coincidentprojectededge.h"
#include "featurefilters/secant.h"

#include "featurefilters/faceadjacenttoedges.h"
#include "featurefilters/faceadjacenttofaces.h"
#include "featurefilters/facetopology.h"
#include "featurefilters/boundaryofface.h"

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
