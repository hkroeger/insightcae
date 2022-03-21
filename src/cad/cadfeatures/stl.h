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

#ifndef STL_H
#define STL_H

#include "cadfeature.h"
#include "MeshVS_Mesh.hxx"

#include "boost/variant.hpp"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

namespace insight {
namespace cad {


/**
 * @brief The STL class
 * loads and displays a triangulated surface.
 * The surfaces cannot be modified.
 */
class STL
    : public Feature
{
public:

  typedef boost::variant<
    boost::filesystem::path,
    vtkSmartPointer<vtkPolyData>
  > GeometrySpecification;

  typedef boost::variant<
    boost::blank,
    gp_Trsf,
    FeaturePtr
  > TransformationSpecification;

private:
  GeometrySpecification geometry_;
  TransformationSpecification transform_;

  STL(GeometrySpecification geometry);

  STL(GeometrySpecification geometry,
      TransformationSpecification transform);


protected:
    Handle_Poly_Triangulation aSTLMesh_;
    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType("STL");
    STL();

    static FeaturePtr create
    (
        GeometrySpecification geometry
    );
    static FeaturePtr create_trsf
    (
        GeometrySpecification geometry,
        TransformationSpecification transform
    );


    virtual void insertrule(parser::ISCADParser& ruleset) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;

};


}
}

#endif // STL_H
