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
#include "channel.h"

#include <sstream>

#include <boost/assign.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include <boost/assign/ptr_list_of.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

#include "base/factory.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
  
  
defineType(FreeShearFlow);
addToFactoryTable(Analysis, FreeShearFlow, NoParameters);

FreeShearFlow::FreeShearFlow(const NoParameters&)
: OpenFOAMAnalysis("Free Shear Flow", "LES of Free Shear Flow")
{
}

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
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", hs);
  PSDBL(p, "geometry", W);
  PSDBL(p, "geometry", L);

  PSDBL(p, "operation", v);
  PSDBL(p, "operation", lambda);
  PSDBL(p, "fluid", nu);

  in_upper_="inletA";
  in_lower_="inletB";
  outlet_="outlet";
  far_upper_="farA";
  far_lower_="farB";
  cycl_prefix_="cycl";
  
  double ReA=0.5*H*v/nu;
  double RetauA=ChannelBase::Retau(ReA);
  double utauA=RetauA*nu/(0.5*H);
  std::cout<<"ReA="<<ReA<<", RetauA="<<RetauA<<", utauA="<<utauA<<std::endl;
  
  
  double ReB=0.5*H*v*lambda/nu;
  double RetauB=ChannelBase::Retau(ReB);
  double utauB=RetauB*nu/(0.5*H);
  std::cout<<"ReB="<<ReB<<", RetauB="<<RetauB<<", utauB="<<utauB<<std::endl;
}


void FreeShearFlow::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", hs);
  PSDBL(p, "geometry", W);
  PSDBL(p, "geometry", L);
  PSINT(p, "mesh", nx);
  PSDBL(p, "mesh", gradax);
  PSDBL(p, "mesh", gradh);
  
  double dx=L/double(nx);
  int nh=std::max(1, int(round(H/dx)));
  int nhs=std::max(1, int(round(hs/dx)));
  int nw=std::max(1, int(round(W/dx)));
  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");

  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0, -0.5*hs, 0))
      (1, 	vec3(L, -0.5*hs, 0))
      (2, 	vec3(L, 0.5*hs, 0))
      (3, 	vec3(0, 0.5*hs, 0))
      (4, 	vec3(0, 0.5*H, 0))
      (5, 	vec3(L, 0.5*H, 0))
      (40, 	vec3(0, -0.5*H, 0))
      (50, 	vec3(L, -0.5*H, 0))
      .convert_to_container<std::map<int, Point> >()
  ;
  
  // create patches
  Patch& inu= 	bmd->addPatch(in_upper_, new Patch());
  Patch& inl= 	bmd->addPatch(in_lower_, new Patch());
  Patch& out= 	bmd->addPatch(outlet_, new Patch());
  Patch& faru= 	bmd->addPatch(far_upper_, new Patch("symmetryPlane"));
  Patch& farl= 	bmd->addPatch(far_lower_, new Patch("symmetryPlane"));
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  string side_type="cyclic";
//   if (p.getBool("mesh/2d")) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch(cycl_prefix_, new Patch(side_type));
  
  arma::mat vH=vec3(0, 0, W);

#define PTS(a,b,c,d) \
  P_8(pts[a], pts[b], pts[c], pts[d], \
      pts[a]+vH, pts[b]+vH, pts[c]+vH, pts[d]+vH)
      
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,2,3),
	nx, nhs, nw,
	list_of<double>(gradax)(1.)(1.)
      )
    );
    out.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
  }
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(3,2,5,4),
	nx, nh, nw,
	list_of<double>(gradax)(gradh)(1.)
      )
    );
    inu.addFace(bl.face("0473"));
    out.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
    faru.addFace(bl.face("2376"));
  }
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(40,50,1,0),
	nx, nh, nw,
	list_of<double>(gradax)(1./gradh)(1.)
      )
    );
    out.addFace(bl.face("1265"));
    inl.addFace(bl.face("0473"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
    farl.addFace(bl.face("0154"));
  }

  cycl_side.appendPatch(cycl_side_0);
  cycl_side.appendPatch(cycl_side_1);
  
  cm.insert(bmd.release());

  cm.createOnDisk(executionPath());
  cm.executeCommand(executionPath(), "blockMesh"); 
}


void FreeShearFlow::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
}

}