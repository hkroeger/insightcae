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

#include "ercoftac_squaresection180degreebend.h"

#include "openfoam/numericscaseelements.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"

#include "channel.h"

#include "boost/filesystem.hpp"
#include "boost/assign.hpp"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{

defineType(ERCOFTAC_SquareSection180DegreeBend);
addToFactoryTable(Analysis, ERCOFTAC_SquareSection180DegreeBend, NoParameters);

ERCOFTAC_SquareSection180DegreeBend::ERCOFTAC_SquareSection180DegreeBend(const NoParameters&)
: OpenFOAMAnalysis(typeName, "Test case for turbulence modelling")
{

}

insight::ParameterSet ERCOFTAC_SquareSection180DegreeBend::defaultParameters() const
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
	    ("LoutByD",	new DoubleParameter(2, "length of outlet extension, divided by D"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))

      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nh",	new IntParameter(64, "# cells in vertical direction"))
	    ("ypluswall", new DoubleParameter(2, "yPlus at the wall grid layer"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;}

void ERCOFTAC_SquareSection180DegreeBend::calcDerivedInputData(const insight::ParameterSet& p)
{
  PSDBL(p, "mesh", nh);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "geometry", LoutByD); 

  D_=88.9;
  LinByD_=1.0;
  HeByD_=3.337;
  nu_=1.72e-5;
  Wb_=11.0;
  
  double Re=1e-3*D_*Wb_/nu_;
  double Re_tau=ChannelBase::Retau(Re);
  double ywall_ = ypluswall/Re_tau;
  grady_=bmd::GradingAnalyzer(ywall_, 0.5*1e-3*D_, nh/2).grad();
  double delta_center=1e3*grady_*ywall_;
  naxi_=LinByD_*D_/delta_center;
  naxo_=LoutByD*D_/delta_center;
  nbend_=M_PI*HeByD_*D_/delta_center;
}

void ERCOFTAC_SquareSection180DegreeBend::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  PSDBL(p, "geometry", LoutByD); 
  double 
    Lout=D_*LoutByD, 
    Lin=D_*LinByD_,
    R=HeByD_*D_;
  
  PSDBL(p, "mesh", nh);
  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0e-3);
  bmd->setDefaultPatch("walls", "wall");
  
    
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(-0.5*D_, -R,  -Lin))
      (1, 	vec3(-0.5*D_, -R,   0.0))
      (2, 	vec3(-0.5*D_,  R,   0.0))
      (3, 	vec3(-0.5*D_,  R, -Lout))

      (4, 	vec3(-0.5*D_, -R-0.5*D_,  -Lin))
      (5, 	vec3(-0.5*D_, -R-0.5*D_,   0.0))
      (6, 	vec3(-0.5*D_,  R+0.5*D_,   0.0))
      (7, 	vec3(-0.5*D_,  R+0.5*D_, -Lout))

      (8, 	vec3(-0.5*D_, -R+0.5*D_,  -Lin))
      (9, 	vec3(-0.5*D_, -R+0.5*D_,   0.0))
      (10, 	vec3(-0.5*D_,  R-0.5*D_,   0.0))
      (11, 	vec3(-0.5*D_,  R-0.5*D_, -Lout))
      ;
  
  // create patches
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  
  int nh2=nh/2;
  
  double grads[]={1./grady_, grady_};
  for (int i=0; i<2; i++)
  {
    arma::mat vH0=vec3(double(i)*0.5*D_, 0, 0);
    arma::mat vH1=vec3(double(i+1)*0.5*D_, 0, 0);
    
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[0])+vH0, (pts[1])+vH0, (pts[5])+vH0, (pts[5])+vH0,
	  (pts[0])+vH1, (pts[1])+vH1, (pts[5])+vH1, (pts[4])+vH1
	  ),
	  naxi_, nh2, nh2,
	  list_of<double>(1.)(1./grady_)(grads[i])
	)
      );
//       in.addFace(bl.face("1265"));
    }

    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[8])+vH0, (pts[9])+vH0, (pts[1])+vH0, (pts[0])+vH0,
	  (pts[8])+vH1, (pts[9])+vH1, (pts[1])+vH1, (pts[0])+vH1
	  ),
	  naxi_, nh2, nh2,
	  list_of<double>(1.)(grady_)(grads[i])
	)
      );
//       in.addFace(bl.face("1265"));
    }
    
  }
  
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");
}

void ERCOFTAC_SquareSection180DegreeBend::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}




insight::ResultSetPtr ERCOFTAC_SquareSection180DegreeBend::evaluateResults(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}

}