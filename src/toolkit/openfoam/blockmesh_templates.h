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


#ifndef INSIGHT_BLOCKMESH_TEMPLATES_H
#define INSIGHT_BLOCKMESH_TEMPLATES_H

#include "openfoam/blockmesh.h"

namespace insight {
  
namespace bmd
{
  
class BlockMeshTemplate
{
};
  
/**
 * A cylinder meshed with an O-grid
 */
class Cylinder
: public BlockMeshTemplate
{
public:
#include "blockmesh_templates__Cylinder__Parameters.h"
/*
PARAMETERSET>>> Cylinder Parameters

geometry = set
{
  D = double 1.0 "Diameter"
  L = double 1.0 "Length"
  ex = vector(0 0 1) "Axial direction"
  er = vector(1 0 0) "Radial direction"
}

mesh = set
{
  nx = int 50 "# cells in axial direction"
  nr = int 10 "# cells in radial direction (from edge of core block to outer radius)"
  nu = int 10 "# cells in circumferential direction (in one of four segments)"
  gradr = double 1 "grading towards outer boundary"
}

<<<PARAMETERSET
*/

protected:
  blockMesh& bmd_;
  Parameters p_;

public:
  Cylinder
  (
    blockMesh& bmd,
    const ParameterSet& p,
    Patch* lowxendPatch=NULL,
    Patch* highxendPatch=NULL,
    Patch* outerPatch=NULL
  );
  
  inline static ParameterSet defaultParameters() 
    { return Parameters::makeDefault(); };
    
  double rCore() const;
};

}

}

#endif