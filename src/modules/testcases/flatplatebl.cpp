/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#include "flatplatebl.h"
#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight 
{
  
defineType(FlatPlateBL);
addToFactoryTable(Analysis, FlatPlateBL, NoParameters);
  
ParameterSet FlatPlateBL::defaultParameters() const
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
	    ("HBydeltae",		new DoubleParameter(10.0, "Domain height above plate, divided by final BL thickness"))
	    ("WBydeltae",		new DoubleParameter(10.0, "Domain height above plate, divided by final BL thickness"))
	    ("L",		new DoubleParameter(12.0, "[m] Length of the domain"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the domain"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nh",	new IntParameter(20, "# cells in vertical direction"))
	    ("ypluswall",	new DoubleParameter(0.5, "yPlus of first cell at the wall grid layer at the final station"))
	    ("dxplus",	new DoubleParameter(60, "lateral mesh spacing at the final station"))
	    ("dzplus",	new DoubleParameter(15, "streamwise mesh spacing at the final station"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Re_L",		new DoubleParameter(1000, "[-] Reynolds number, formulated with final running length"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nu",	new DoubleParameter(1.8e-5, "[m^2/s] Viscosity of the fluid"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))

      ("evaluation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("inittime",	new DoubleParameter(10, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
	    ("meantime",	new DoubleParameter(10, "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"))
	    ("mean2time",	new DoubleParameter(10, "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Options for statistical evaluation"
	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}

FlatPlateBL::FlatPlateBL(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Flat Plate Boundary Layer Test Case",
    "Flat Plate with Evolving Boundary Layer"
  )
{}

FlatPlateBL::~FlatPlateBL()
{

}

void FlatPlateBL::calcDerivedInputData(const ParameterSet& p)
{
  insight::OpenFOAMAnalysis::calcDerivedInputData(p);
  
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
  in_="inlet";
  out_="outlet";
  top_="top";
  cycl_prefix_="cyclic";
  
  Cw_=FlatPlateBL::cw(Re_L);
  cout<<"cw="<<Cw_<<endl;
  delta2e_ = 0.5*Cw_*L;
  cout<<"delta2e="<<delta2e_<<endl;
  H_=HBydeltae*delta2e_;
  cout<<"H="<<H_<<endl;
  W_=WBydeltae*delta2e_;
  cout<<"W="<<W_<<endl;
  Re_theta2e_=Re_L*(delta2e_/L);
  cout<<"Re_theta2e="<<Re_theta2e_<<endl;
  uinf_=Re_L*nu/L;
  cout<<"uinf="<<uinf_<<endl;
  
  cout<<"cf_e="<<cf(Re_L)<<endl;
  ypfac_e_=sqrt(cf(Re_L)/2.)*uinf_/nu;
  cout<<"ypfac="<<ypfac_e_<<endl;
  deltaywall_e_=ypluswall/ypfac_e_;
  cout<<"deltaywall_e="<<deltaywall_e_<<endl;
  gradh_=bmd::GradingAnalyzer(deltaywall_e_, H_, nh).grad();
  cout<<"gradh="<<gradh_<<endl;
  nax_=std::max(1, int(round(L/(dxplus/ypfac_e_))));
  cout<<"nax="<<nax_<<" "<<(dxplus/ypfac_e_)<<endl;
  nlat_=std::max(1, int(round(W_/(dzplus/ypfac_e_))));
  cout<<"nlat="<<nlat_<<" "<<(dzplus/ypfac_e_)<<endl;
}

void FlatPlateBL::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");

  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0, 0., 0))
      (1, 	vec3(L, 0., 0))
      (2, 	vec3(L, H_, 0))
      (3, 	vec3(0, H_, 0))
  ;
  
  // create patches
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  Patch& top= 	bmd->addPatch(top_, new Patch("symmetryPlane"));
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  std::string side_type="cyclic";
//   if (p.getBool("mesh/2d")) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch(cycl_prefix_, new Patch(side_type));
  
  arma::mat vH=vec3(0, 0, W_);

#define PTS(a,b,c,d) \
  P_8(pts[a], pts[b], pts[c], pts[d], \
      pts[a]+vH, pts[b]+vH, pts[c]+vH, pts[d]+vH)
      
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,2,3),
	nax_, nh, nlat_,
	list_of<double>(1.)(gradh_)(1.)
      )
    );
    in.addFace(bl.face("0473"));
    out.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
  }

  cycl_side.appendPatch(cycl_side_0);
  cycl_side.appendPatch(cycl_side_1);
  
  cm.insert(bmd.release());

  cm.createOnDisk(executionPath());
  cm.executeCommand(executionPath(), "blockMesh"); 
}


void FlatPlateBL::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}


insight::ResultSetPtr FlatPlateBL::evaluateResults(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}

insight::Analysis* FlatPlateBL::clone()
{
  return new FlatPlateBL(NoParameters());
}

double FlatPlateBL::G(double Alpha, double D)
{
  struct Obj: public Objective1D
  {
    double Alpha, D;
    virtual double operator()(double G) const 
    { 
//       cout << G << (1./G) + 2.*log(1./G) - D - Alpha <<endl;
      return (Alpha/G) + 2.*log(Alpha/G) - D - Alpha; 
    }
  } obj;
  obj.Alpha=Alpha;
  obj.D=D;
  return nonlinearSolve1D(obj, 1e-6, 1e3*Alpha);
}

double FlatPlateBL::cw(double Re, double Cplus)
{
  return 2.*pow( (0.41/log(Re)) * G( log(Re), 2.*log(0.41)+0.41*(Cplus-3.) ), 2);
}

double FlatPlateBL::cf(double Rex, double Cplus)
{
  struct Obj: public Objective1D
  {
    double Rex, Cplus;
    virtual double operator()(double gamma) const 
    { 
      return (1./gamma) -(1./0.41)*log(gamma*gamma*Rex)
	- Cplus - (1./0.41)*(2.*0.55-log(3.78)); 
    }
  } obj;
  obj.Rex=Rex;
  obj.Cplus=Cplus;
  double gamma=nonlinearSolve1D(obj, 1e-7, 10.);
  return 2.*gamma*gamma;
}

}