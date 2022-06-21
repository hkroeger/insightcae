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

#include "cadtypes.h"
#include "cadfeature.h"

#include "cadfeatures/compound.h"

namespace insight 
{
namespace cad 
{


OCCException::OCCException(const std::string message)
    : insight::Exception(message)
{
}

OCCException& OCCException::addInvolvedShape(const std::string& label, TopoDS_Shape shape)
{
    involvedShapes_[label]=shape;
    return *this;
}

OCCException& OCCException::addInvolvedShape(TopoDS_Shape shape)
{
    involvedShapes_[str(boost::format("involvedShape_%d")%(involvedShapes_.size()))]=shape;
    return *this;
}

const std::map<std::string, TopoDS_Shape>& OCCException::involvedShapes() const
{
    return involvedShapes_;
}

void OCCException::saveInvolvedShapes(const boost::filesystem::path& outFile) const
{
    CompoundFeatureMap m;
    std::transform(
                involvedShapes_.begin(),
                involvedShapes_.end(),
                std::inserter(m, m.begin()),
                [&](const InvolvedShapesList::value_type& iv)
                {
                    return CompoundFeatureMap::value_type(iv.first, Feature::CreateFromShape(iv.second));
                }
    );
    auto cc=Compound::create_map(m);
    cc->saveAs(outFile);
}

CADException::CADException(ConstFeaturePtr feat, const std::string message)
: OCCException(
    (feat ? "In feature "+feat->featureSymbolName()+": " : "without feature context: " )
    + message),
  errorfeat_(feat)
{}

}
}
