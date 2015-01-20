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
#include "openfoam/boundaryconditioncaseelements.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"

#include "channel.h"
#include "refdata.h"

#include "base/boost_include.h"

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
  
  return p;
  
}

void ERCOFTAC_SquareSection180DegreeBend::calcDerivedInputData()
{
  const ParameterSet& p=*parameters_;
  PSINT(p, "mesh", nh);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "geometry", LoutByD); 
  
  in_="inlet";
  out_="outlet";

  D_=88.9;
  LinByD_=1.0;
  HeByD_=3.357;
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

void ERCOFTAC_SquareSection180DegreeBend::createMesh(insight::OpenFOAMCase& cm)
{
  PSDBL(p(), "geometry", LoutByD); 
  double 
    Lout=D_*LoutByD, 
    Lin=D_*LinByD_,
    R=HeByD_*D_;
  
  PSINT(p(), "mesh", nh);
  
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
      .convert_to_container<std::map<int, Point> >();
  
  // create patches
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  
  int nh2=nh/2;
  
  double grads[]={grady_, 1./grady_};
  for (int i=0; i<2; i++)
  {
    arma::mat vH0=vec3(double(i)*0.5*D_, 0, 0);
    arma::mat vH1=vec3(double(i+1)*0.5*D_, 0, 0);

    // inlet extension
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[0])+vH0, (pts[1])+vH0, (pts[5])+vH0, (pts[4])+vH0,
	  (pts[0])+vH1, (pts[1])+vH1, (pts[5])+vH1, (pts[4])+vH1
	  ),
	  naxi_, nh2, nh2,
	  list_of<double>(1.)(1./grady_)(grads[i])
	)
      );
      in.addFace(bl.face("0473"));
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
      in.addFace(bl.face("0473"));
    }

    // channel bend
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[9])+vH0, (pts[10])+vH0, (pts[2])+vH0, (pts[1])+vH0,
	  (pts[9])+vH1, (pts[10])+vH1, (pts[2])+vH1, (pts[1])+vH1
	  ),
	  nbend_, nh2, nh2,
	  list_of<double>(1.)(grady_)(grads[i])
	)
      );
    }

    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[1])+vH0, (pts[2])+vH0, (pts[6])+vH0, (pts[5])+vH0,
	  (pts[1])+vH1, (pts[2])+vH1, (pts[6])+vH1, (pts[5])+vH1
	  ),
	  nbend_, nh2, nh2,
	  list_of<double>(1.)(1./grady_)(grads[i])
	)
      );
    }

    // outlet extension
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[3])+vH0, (pts[2])+vH0, (pts[10])+vH0, (pts[11])+vH0,
	  (pts[3])+vH1, (pts[2])+vH1, (pts[10])+vH1, (pts[11])+vH1
	  ),
	  naxo_, nh2, nh2,
	  list_of<double>(1.)(1./grady_)(grads[i])
	)
      );
      out.addFace(bl.face("0473"));
    }

    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  (pts[7])+vH0, (pts[6])+vH0, (pts[2])+vH0, (pts[3])+vH0,
	  (pts[7])+vH1, (pts[6])+vH1, (pts[2])+vH1, (pts[3])+vH1
	  ),
	  naxo_, nh2, nh2,
	  list_of<double>(1.)(grady_)(grads[i])
	)
      );
     out.addFace(bl.face("0473"));
    }
    
  }

  for (int i=0; i<3; i++)
  {
    arma::mat vH=vec3(double(i)*0.5*D_, 0, 0);
    bmd->addEdge(new ArcEdge(vH+pts[1], vH+pts[2], vH+vec3(-0.5*D_,0,R)));
    bmd->addEdge(new ArcEdge(vH+pts[5], vH+pts[6], vH+vec3(-0.5*D_,0,R+0.5*D_)));
    bmd->addEdge(new ArcEdge(vH+pts[9], vH+pts[10], vH+vec3(-0.5*D_,0,R-0.5*D_)));
  }
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");
}

void ERCOFTAC_SquareSection180DegreeBend::createCase(insight::OpenFOAMCase& cm)
{
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert( new simpleFoamNumerics(cm, simpleFoamNumerics::Parameters()
  ) );
  
  arma::mat udata=refdatalib.getProfile("ERCOFTAC_SX180Bend", "in/umean_vs_x_y");
  arma::mat vdata=refdatalib.getProfile("ERCOFTAC_SX180Bend", "in/vmean_vs_x_y");
  arma::mat wdata=refdatalib.getProfile("ERCOFTAC_SX180Bend", "in/wmean_vs_x_y");
  arma::mat kdata=refdatalib.getProfile("ERCOFTAC_SX180Bend", "in/tke_vs_x_y");
  arma::mat edata=refdatalib.getProfile("ERCOFTAC_SX180Bend", "in/epsilon_vs_x_y");
  // all point locations are the same!
  
  double ztol=1e-3*0.1*D_;
  double zin=LinByD_*D_*1e-3;
  arma::mat xyz( arma::join_rows( arma::join_rows( 
    udata.col(0)-0.5*1e-3*D_, -udata.col(1)), -(zin+0.5*ztol)*arma::ones(udata.n_rows,1) 
  ));

  // Mirror
  arma::mat xyz2(xyz);
  xyz2.col(0)=0.5*1e-3*D_-(xyz.col(0)+0.5*1e-3*D_);
  xyz=join_cols(xyz, xyz2);
  
  //make two layers for proper interpolation
  arma::mat xyz3(xyz);
  xyz3.col(2)+=ztol;
  xyz=join_cols(xyz, xyz3);
  
  arma::mat uvw(arma::join_rows(arma::join_rows( udata.col(2), vdata.col(2) ), wdata.col(2)));
  arma::mat uvw2(uvw);
  uvw2.col(0)*=-1;
  uvw=join_cols(uvw, uvw2);
  arma::mat uvw3(uvw);
  uvw=join_cols(uvw, uvw3);

  arma::mat tke=join_cols(kdata.col(2),kdata.col(2));
  tke=join_cols(tke, tke);
  
  arma::mat epsilon=join_cols(edata.col(2), edata.col(2));
  epsilon=join_cols(epsilon, epsilon);

  // clip k smaller threshold
  double thr=1e-10;
  for (int j=0; j<tke.n_rows; j++)
  {
    if (tke(j)<thr) tke(j)=thr;
    if (epsilon(j)<thr) epsilon(j)=thr;
  }
  
  cm.insert(new ExptDataInletBC(cm, in_, boundaryDict, ExptDataInletBC::Parameters()
    .set_points(xyz)
    .set_velocity(uvw)
    .set_TKE(tke)
    .set_epsilon(epsilon)
  ));
  
  cm.insert(new PressureOutletBC(cm, out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0)
  ));
  
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu_) ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p().get<SelectionParameter>("fluid/turbulenceModel").selection());
}




insight::ResultSetPtr ERCOFTAC_SquareSection180DegreeBend::evaluateResults(insight::OpenFOAMCase& cm)
{

}

}