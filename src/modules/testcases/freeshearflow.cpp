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

#include "freeshearflow.h"
#include "base/factory.h"
#include "base/plottools.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/snappyhexmesh.h"

#include <sstream>

#include <boost/assign.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include <boost/assign/ptr_list_of.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{

FreeShearFlow::FreeShearFlow(const std::string& name, const std::string& description)
: OpenFOAMAnalysis(name, description)
{
}

insight::ParameterSet FreeShearFlow::defaultParameters() const
{
  ParameterSet p(OpenFOAMAnalysis::defaultParameters());
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    
      ("geometry", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("L",		new DoubleParameter(1.0, "[m] length of domain"))
	    ("H",		new DoubleParameter(10.0, "[m] total height of domain"))
	    ("W",		new DoubleParameter(1.0, "[m] total width of domain"))
	    ("hs",		new DoubleParameter(0.001, "[m] height of splitter plate"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the propeller"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nx",	new IntParameter(40, "# cells axial"))
	    ("gradax",	new DoubleParameter(10, "axial grading"))
	    ("gradh",	new DoubleParameter(10, "vertical grading"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("v",		new DoubleParameter(1.0, "[m/s] velocity above"))
	    ("lambda",		new DoubleParameter(0.5, "ratio of velocity below"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("rho",	new DoubleParameter(1.0, "[kg/m^3] Density of the fluid"))
	    ("nu",	new DoubleParameter(1e-6, "[m^2/s] Viscosity of the fluid"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;  
}

void FreeShearFlow::calcDerivedInputData(const insight::ParameterSet& p)
{
  in_upper_="inletA";
  in_lower_="inletB";
  outlet_="outlet";
  far_="far";
  cycl_front_="cycl_m";
  cycl_back_="cylc_p";
}


void FreeShearFlow::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", hs);
  PSDBL(p, "geometry", W);
  PSDBL(p, "geometry", L);
  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");

  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0, -0.5*hs, 0))
      (1, 	vec3(-0.5*L, -0.5*H, -0.5*B))
      (2, 	vec3(-0.5*L, -0.5*H, 0.5*B))
      (3, 	vec3(0.5*L, -0.5*H, 0.5*B))
  ;
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  string side_type="cyclic";
  if (p.getBool("mesh/2d")) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch("cycl_side", new Patch(side_type));
  
  arma::mat vHbuf=vec3(0, 0, 0);
  arma::mat vH=vec3(0, H, 0);
  int nh=nh_;
  
  if (nhbuf_>0)
  {
    vHbuf=vec3(0, hbuf_, 0);
    vH=vec3(0, H-2.*hbuf_, 0);
    nh=nh_-nhbuf_;
    
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  pts[0], pts[1], pts[2], pts[3],
	  (pts[0])+vHbuf, (pts[1])+vHbuf, (pts[2])+vHbuf, (pts[3])+vHbuf
	  ),
	  nax_, nb_, nhbuf_,
	  list_of<double>(1.)(1.)(1.)
	)
      );
      cycl_out.addFace(bl.face("0473"));
      cycl_in.addFace(bl.face("1265"));
      cycl_side_0.addFace(bl.face("0154"));
      cycl_side_1.addFace(bl.face("2376"));
    }

    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	    (pts[0])+vH+vHbuf, (pts[1])+vH+vHbuf, (pts[2])+vH+vHbuf, (pts[3])+vH+vHbuf,
	    (pts[0])+vH+2.*vHbuf, (pts[1])+vH+2.*vHbuf, (pts[2])+vH+2.*vHbuf, (pts[3])+vH+2.*vHbuf
	  ),
	  nax_, nb_, nhbuf_,
	  list_of<double>(1.)(1.)(1.)
	)
      );
      cycl_out.addFace(bl.face("0473"));
      cycl_in.addFace(bl.face("1265"));
      cycl_side_0.addFace(bl.face("0154"));
      cycl_side_1.addFace(bl.face("2376"));
    }
    
  }

  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  pts[0]+vHbuf, pts[1]+vHbuf, pts[2]+vHbuf, pts[3]+vHbuf,
	  (pts[0])+0.5*vH+vHbuf, (pts[1])+0.5*vH+vHbuf, (pts[2])+0.5*vH+vHbuf, (pts[3])+0.5*vH+vHbuf
	),
	nax_, nb_, nh,
	list_of<double>(1.)(1.)(gradh_)
      )
    );
    cycl_out.addFace(bl.face("0473"));
    cycl_in.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0154"));
    cycl_side_1.addFace(bl.face("2376"));
  }

  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  (pts[0])+0.5*vH+vHbuf, (pts[1])+0.5*vH+vHbuf, (pts[2])+0.5*vH+vHbuf, (pts[3])+0.5*vH+vHbuf,
	  (pts[0])+vH+vHbuf, (pts[1])+vH+vHbuf, (pts[2])+vH+vHbuf, (pts[3])+vH+vHbuf
	),
	nax_, nb_, nh,
	list_of<double>(1.)(1.)(1./gradh_)
      )
    );
    cycl_out.addFace(bl.face("0473"));
    cycl_in.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0154"));
    cycl_side_1.addFace(bl.face("2376"));
  }

  cycl_side.appendPatch(cycl_side_0);
  cycl_side.appendPatch(cycl_side_1);
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh"); 
}


void FreeShearFlow::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}

}