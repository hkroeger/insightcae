/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "channel.h"

#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "refdata.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/ptr_container/ptr_container.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

#include "gnuplot-iostream.h"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{


defineType(ChannelBase);

ChannelBase::ChannelBase(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Channel Flow Test Case",
    "Cylindrical domain with cyclic BCs on axial ends"
  ),
  cycl_in_("cycl_half0"),
  cycl_out_("cycl_half1")
{
}

ChannelBase::~ChannelBase()
{

}

ParameterSet ChannelBase::defaultParameters() const
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
	    ("H",		new DoubleParameter(2.0, "[m] Height of the channel"))
	    ("B",		new DoubleParameter(4.19, "[m] Width of the channel"))
	    ("L",		new DoubleParameter(12.56, "[m] Length of the channel"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the bearing"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nax",	new IntParameter(100, "# cells in axial direction"))
	    ("s",	new DoubleParameter(1.0, "Axial grid anisotropy (ratio of axial cell edge length to lateral edge length)"))
	    ("ypluswall",	new DoubleParameter(0.5, "yPlus at the wall grid layer"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Re_tau",		new DoubleParameter(180, "[-] Friction-Velocity-Reynolds number"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("evaluation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("inittime",	new DoubleParameter(5, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
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

std::string ChannelBase::cyclPrefix() const
{
  boost:smatch m;
  boost::regex_search(cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  cout<<namePrefix<<endl;
  return namePrefix;
}

void ChannelBase::calcDerivedInputData(const ParameterSet& p)
{
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);

  PSDBL(p, "mesh", ypluswall);
  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", s);
  
  // Physics
  double kappa=0.41;
  double Cplus=5.0;
  Ubulk_=(1./kappa)*log(Re_tau)+Cplus-1.7;
  Re_=Re_tau*Ubulk_;
  T_=L/Ubulk_;
  nu_=1./Re_tau;
  ywall_ = ypluswall/Re_tau;
  
  // grid
  double Delta=L/double(nax);
  
  nb_=B/(Delta/s);
  gradh_=(Delta/s) / ywall_;
  nh_=max(1, bmd::GradingAnalyzer(gradh_).calc_n(ywall_, H/2.));
}

void ChannelBase::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
      
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0.5*L, -0.5*H, -0.5*B))
      (1, 	vec3(-0.5*L, -0.5*H, -0.5*B))
      (2, 	vec3(-0.5*L, -0.5*H, 0.5*B))
      (3, 	vec3(0.5*L, -0.5*H, 0.5*B))
  ;
  arma::mat vH=vec3(0, H, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  Patch& cycl_side= 	bmd->addPatch("cycl_side", new Patch("cyclic"));
  
  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  pts[0], pts[1], pts[2], pts[3],
	  (pts[0])+0.5*vH, (pts[1])+0.5*vH, (pts[2])+0.5*vH, (pts[3])+0.5*vH
	),
	nax, nb_, nh_,
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
	  (pts[0])+0.5*vH, (pts[1])+0.5*vH, (pts[2])+0.5*vH, (pts[3])+0.5*vH,
	  (pts[0])+1*vH, (pts[1])+1*vH, (pts[2])+1*vH, (pts[3])+1*vH
	),
	nax, nb_, nh_,
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


void ChannelBase::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  
  cm.insert(new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters().set_LES(true) ) );
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_name("averaging")
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(inittime*T_)
  ));
  
//   cm.insert(new RadialTPCArray(cm, RadialTPCArray::Parameters()
//     .set_name_prefix("tpc_interior")
//     .set_R(0.5*D)
//     .set_x(0.5*L)
//     .set_axSpan(0.5*L)
//     .set_tanSpan(M_PI)
//     .set_timeStart( (inittime+meantime)*T )
//   ));
  

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu_) ));
  cm.insert(new CyclicPairBC(cm, "cycl_side", boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

}


  
ResultSetPtr ChannelBase::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  
  boost::ptr_vector<sampleOps::set> sets;
  
  double x=0.0;
  sets.push_back(new sampleOps::linearAveragedUniformLine(sampleOps::linearAveragedUniformLine::Parameters()
    .set_name("radial")
    .set_start( vec3(-0.49*L, 0.005* 0.5*H, -0.49*B))
    .set_end(   vec3(-0.49*L, 0.997* 0.5*H, -0.49*B))
    .set_dir1(vec3(0.98*L,0,0))
    .set_dir2(vec3(0,0,0.98*B))
    .set_nd1(5)
    .set_nd2(5)
  ));
  
  sample(cm, executionPath(), 
     list_of<std::string>("p")("U")("UMean")("UPrime2Mean"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data = static_cast<sampleOps::linearAveragedUniformLine&>(*sets.begin())
    .readSamples(cm, executionPath(), &cd);
    
    
  // Mean velocity profiles
  {
    Gnuplot gp;
    string chart_name="mean_velocity";
    string chart_file_name=chart_name+".png";
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y+'; set ylabel '<U+>'; set grid; ";
    gp<<"set logscale x;";
    
    int c=cd["UMean"].col;
    
    arma::mat refdata_umean, refdata_wmean;
    refdata_umean=refdatalib.getProfile("MKM_Channel", "180/umean_vs_yp");
    refdata_wmean=refdatalib.getProfile("MKM_Channel", "180/wmean_vs_yp");
    
    double fac_yp=Re_tau;
    double fac_Up=1.0;
    
    gp<<"plot 0 not lt -1,"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Axial',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Spanwise',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Wall normal',"
	
	" '-' w p t 'Axial (DNS, MKM)',"
	" '-' w p t 'Spanwise (DNS, MKM)'"
	<<endl;
	
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+1))) );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+2))) );
    gp.send1d( refdata_umean );
    gp.send1d( refdata_wmean );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Wall normal profiles of averaged velocities", ""
    )));
    
  }
  
  // Mean reynolds stress profiles
  {
    Gnuplot gp;
    string chart_name="mean_Rstress";
    string chart_file_name=chart_name+".png";
    
    double fac_yp=Re_tau;
    double fac_Rp=1.;
    
    int c=cd["UPrime2Mean"].col;
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y+'; set ylabel '<R+>'; set grid; ";
    gp<<"set logscale x;";
    gp<<"set yrange [:"<<fac_Rp*max(data.col(c))<<"];";
    
    arma::mat refdata_Ruu, refdata_Rvv, refdata_Rww;
    refdata_Ruu=refdatalib.getProfile("MKM_Channel", "180/Ruu_vs_yp");
    refdata_Rvv=refdatalib.getProfile("MKM_Channel", "180/Rvv_vs_yp");
    refdata_Rww=refdatalib.getProfile("MKM_Channel", "180/Rww_vs_yp");
    
    gp<<"plot 0 not lt -1,"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rxx (Axial)',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Ryy (Circumferential)',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rzz (Radial)',"
	" '-' w p t 'Rxx (DNS, MKM)',"
	" '-' w p t 'Ryy (DNS, MKM)',"
	" '-' w p t 'Rzz (DNS, MKM)'"
	<<endl;
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+3))) );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+5))) );
    gp.send1d( refdata_Ruu );
    gp.send1d( refdata_Rvv );
    gp.send1d( refdata_Rww );
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Wall normal profiles of averaged reynolds stresses", ""
    )));
    
  }
/*
  const RadialTPCArray* tpcs=cm.get<RadialTPCArray>("tpc_interiorRadialTPCArray");
  if (!tpcs)
    throw insight::Exception("tpc FO array not found in case!");
  tpcs->evaluate(cm, executionPath(), results);
  */

  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
      "eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
      "Show(eb)\n"
      "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
      "setCam([0,0,10], [0,0,0], [0,1,0])\n"
      "WriteImage('pressure_longi.jpg')\n"
    )
  );
  results->insert("pressureContour",
    std::auto_ptr<Image>(new Image
    (
    "pressure_longi.jpg", 
    "Contour of pressure (longitudinal section)", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
	"Show(eb)\n"
	"displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	"setCam([0,0,10], [0,0,0], [0,1,0])\n"
	"WriteImage('U"+c+"_longi.jpg')\n"
      )
    );
    results->insert("U"+c+"Contour",
      std::auto_ptr<Image>(new Image
      (
      "U"+c+"_longi.jpg", 
      "Contour of "+c+"-Velocity", ""
    )));
  }
  
  return results;
}




defineType(ChannelCyclic);

ChannelCyclic::ChannelCyclic(const NoParameters& nop)
: ChannelBase(nop)
{
}

void ChannelCyclic::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  ChannelBase::createMesh(cm, p);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelCyclic::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  ChannelBase::createCase(cm, p);
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(Ubulk_, 0, 0))
		));
/*  
  int n_r=10;
  for (int i=1; i<n_r; i++) // omit first and last
  {
    double x = double(i)/(n_r);
    double r = -cos(M_PI*(0.5+0.5*x))*0.5*D;
    cout<<"Inserting tpc FO at r="<<r<<endl;
    
    cm.insert(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
      .set_name("tpc_tan_"+lexical_cast<string>(i))
      .set_outputControl("timeStep")
      .set_p0(vec3(r, 0, 0.5*L))
      .set_directionSpan(vec3(0, M_PI, 0)) 
      .set_np(50)
      .set_homogeneousTranslationUnit(vec3(0, M_PI/2., 0))
      .set_nph(8)
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)
      .set_timeStart( (inittime+meantime)*T )
    ));
    
    cm.insert(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
      .set_name("tpc_ax_"+lexical_cast<string>(i))
      .set_outputControl("timeStep")
      .set_p0(vec3(r, 0, 0))
      .set_directionSpan(vec3(0, 0, 0.5*L)) 
      .set_np(50)
      .set_homogeneousTranslationUnit(vec3(0, M_PI/2., 0))
      .set_nph(8)
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)
      .set_timeStart( (inittime+meantime)*T )
    ));
    
  }*/
}

void ChannelCyclic::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "operation", Re_tau);
  /*
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(Ubulk_)+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  cm.executeCommand(executionPath(), "applyBoundaryLayer", list_of<string>("-ybl")(lexical_cast<string>(0.25)) );
  cm.executeCommand(executionPath(), "randomizeVelocity", list_of<string>(lexical_cast<string>(0.1*Ubulk_)) );
  */
  cm.executeCommand(executionPath(), "perturbU", 
		    list_of<string>
		    (lexical_cast<string>(Re_tau))
		    ("("+lexical_cast<string>(Ubulk_)+" 0 0)") 
		   );
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void ChannelCyclic::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  OpenFOAMAnalysis::applyCustomOptions(cm, p, dicts);
  
  OFDictData::dictFile& decomposeParDict=dicts->addDictionaryIfNonexistent("system/decomposeParDict");
  int np=decomposeParDict.getInt("numberOfSubdomains");
  OFDictData::dict msd;
  OFDictData::list dl;
  dl.push_back(np);
  dl.push_back(1);
  dl.push_back(1);
  msd["n"]=dl;
  msd["delta"]=0.001;
  decomposeParDict["method"]="simple";
  decomposeParDict["simpleCoeffs"]=msd;

  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  if (cm.OFversion()<=160)
  {
    controlDict["application"]="channelFoam";
  }
  controlDict["endTime"] = (inittime+meantime+mean2time)*T_;
}

addToFactoryTable(Analysis, ChannelCyclic, NoParameters);



}
