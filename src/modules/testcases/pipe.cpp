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

#include "pipe.h"

#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"

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


  
defineType(PipeBase);

double PipeBase::Re(double Re_tau)
{
  double k=0.41;
  double Cplus=5.0;
  
  return Re_tau*((1./k)*log(Re_tau)+Cplus);
}


double PipeBase::Retau(double Re)
{
  struct Obj: public Objective1D
  {
    double Re;
    virtual double operator()(double x) const { return Re-PipeBase::Re(x); }
  } obj;
  obj.Re=Re;
  return nonlinearSolve1D(obj, 1e-3*Re, Re);
}
  
PipeBase::PipeBase(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Pipe Flow Test Case",
    "Cylindrical domain with cyclic BCs on axial ends"
  ),
  cycl_in_("cycl_half0"),
  cycl_out_("cycl_half1")
{}

PipeBase::~PipeBase()
{

}

ParameterSet PipeBase::defaultParameters() const
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
	    ("D",		new DoubleParameter(2.0, "[m] Diameter of the pipe"))
	    ("L",		new DoubleParameter(12.0, "[m] Length of the pipe"))
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
	    ("x",	new DoubleParameter(0.5, "Edge length of core block as fraction of diameter"))
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

std::string PipeBase::cyclPrefix() const
{
  boost:smatch m;
  boost::regex_search(cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  cout<<namePrefix<<endl;
  return namePrefix;
}


double PipeBase::calcLc(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "mesh", x);
  return x*D;
}

int PipeBase::calcnc(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", x);
  PSDBL(p, "mesh", s);
  double Delta=L/double(nax);
  return D*(M_PI+4.*x)/(8.*Delta/s);
}

int PipeBase::calcnr(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", s);
  PSDBL(p, "mesh", x);
  double Delta=L/double(nax);
  double lr=0.5*D*(1.-sqrt(2.)*x);
  int nr=max(1, bmd::GradingAnalyzer(calcgradr(p)).calc_n(calcywall(p), lr));
  cout<<"n_r="<<nr<<endl;
  return nr;
}

double PipeBase::calcywall(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "operation", Re_tau);
  
  double ywall= ypluswall*0.5*D/Re_tau;
  cout<<"ywall = "<<ywall<<endl;
  return ywall;
}

double PipeBase::calcgradr(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", s);

  double Delta=L/double(nax);
  double delta0=calcywall(p);
  double grad=(Delta/s) / delta0;
  cout<<"Grading = "<<grad<<endl;
  return grad;
}

double PipeBase::calcUtau(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);

  return 1./(0.5*D);
}


double PipeBase::calcRe(const ParameterSet& p) const
{

  PSDBL(p, "operation", D);
  PSDBL(p, "operation", Re_tau);
  double nu=1./Re_tau;
  return calcUbulk(p)*D/nu;
}

double PipeBase::calcUbulk(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "operation", Re_tau);
  double k=0.41;
  double Cplus=5.0;
  
  double nu=1./Re_tau;
  double rho=1.0;
  double tau0= pow(Re_tau*nu*sqrt(rho)/(0.5*D), 2);
  return sqrt(tau0/rho)*(1./k)*log(Re_tau)+Cplus; //calcRe(p)*(1./Re_tau)/D;
}

double PipeBase::calcT(const ParameterSet& p) const
{
  PSDBL(p, "geometry", L);
  return L/calcUbulk(p);
}

void PipeBase::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  
  double Lc=calcLc(p);
  int nc=calcnc(p);
  int nr=calcnr(p);
  double gradr=calcgradr(p);
  cout<<"Lc="<<Lc<<", nc="<<nc<<", nr="<<nr<<", grad_r="<<gradr<<endl;
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (11, 	vec3(0, 0.5*D, 0))
      (10, 	vec3(0,  cos(0.5*al)*Lc, 0.))
      (9, 	vec3(0,  1.2*0.5*Lc, 0.))
  ;
  arma::mat vL=vec3(L, 0, 0);
  arma::mat ax=vec3(1, 0, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  
  {
    arma::mat r0=rotMatrix(0.5*al, ax);
    arma::mat r1=rotMatrix(1.5*al, ax);
    arma::mat r2=rotMatrix(2.5*al, ax);
    arma::mat r3=rotMatrix(3.5*al, ax);

    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  r1*pts[10], r2*pts[10], r3*pts[10], r0*pts[10],
	  (r1*pts[10])+vL, (r2*pts[10])+vL, (r3*pts[10])+vL, (r0*pts[10])+vL
	),
	nc, nc, nax
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));
  }

  for (int i=0; i<4; i++)
  {
    arma::mat r0=rotMatrix(double(i+0.5)*al, ax);
    arma::mat r1=rotMatrix(double(i+1.5)*al, ax);
    
    Block& bl = bmd->addBlock
    (
      new Block(P_8(
	  r1*pts[10], r0*pts[10], r0*pts[11], r1*pts[11],
	  (r1*pts[10])+vL, (r0*pts[10])+vL, (r0*pts[11])+vL, (r1*pts[11])+vL
	),
	nc, nr, nax,
	list_of<double>(1)(1./gradr)(1)
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));

    arma::mat rmid=rotMatrix(double(i+1)*al, ax);
    bmd->addEdge(new ArcEdge(r1*pts[11], r0*pts[11], rmid*pts[11]));
    bmd->addEdge(new ArcEdge((r1*pts[11])+vL, (r0*pts[11])+vL, (rmid*pts[11])+vL));

    //inner core
    bmd->addEdge(new ArcEdge(r1*pts[10], r0*pts[10], rmid*pts[9]));
    bmd->addEdge(new ArcEdge((r1*pts[10])+vL, (r0*pts[10])+vL, (rmid*pts[9])+vL));

  }

  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}


void PipeBase::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  double T=calcT(p);
  cout << "Flow-through time T="<<T<<endl;
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  
  cm.insert(new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters().set_LES(true) ) );
  cm.insert(new cuttingPlane(cm, cuttingPlane::Parameters()
    .set_name("plane")
    .set_basePoint(vec3(0,1e-6,1e-6))
    .set_normal(vec3(0,0,1))
    .set_fields(list_of<string>("p")("U")("UMean")("UPrime2Mean"))
  ));
  
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_name("averaging")
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(inittime*T)
  ));
  
  cm.insert(new RadialTPCArray(cm, RadialTPCArray::Parameters()
    .set_name_prefix("tpc_interior")
    .set_R(0.5*D)
    .set_x(0.5*L)
    .set_axSpan(0.5*L)
    .set_tanSpan(M_PI)
    .set_timeStart( (inittime+meantime)*T )
  ));
  
  /*
  cm.insert(new probes(cm, probes::Parameters()
    .set_name("probes")
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(inittime*T)
    .set_probeLocations(list_of<arma::mat>
      (vec3(0.1*L, 0, 0))
      (vec3(0.33*L, 0, 0))
      (vec3(0.5*L, 0, 0))
      (vec3(0.66*L, 0, 0))
      (vec3(0.9*L, 0, 0))
      (vec3(0.1*L, 0.9*0.5*D, 0))
      (vec3(0.33*L, 0.9*0.5*D, 0))
      (vec3(0.5*L, 0.9*0.5*D, 0))
      (vec3(0.66*L, 0.9*0.5*D, 0))
      (vec3(0.9*L, 0.9*0.5*D, 0))
    )
  ));
  */
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(1./Re_tau) ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

  cout<<"Ubulk="<<calcUbulk(p)<<endl;
}


  
ResultSetPtr PipeBase::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  
  boost::ptr_vector<sampleOps::set> sets;
  
  double x=L*0.5;
  sets.push_back(new sampleOps::circumferentialAveragedUniformLine(sampleOps::circumferentialAveragedUniformLine::Parameters()
    .set_name("radial")
    .set_start( vec3(x, 0,  0.01* 0.5*D))
    .set_end(   vec3(x, 0, 0.997* 0.5*D))
    .set_axis(vec3(1,0,0))
  ));
  
  sample(cm, executionPath(), 
     list_of<std::string>("p")("U")("UMean")("UPrime2Mean"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data = static_cast<sampleOps::circumferentialAveragedUniformLine&>(*sets.begin())
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
    
    double fac_yp=Re_tau*2.0/D;
    double fac_Up=1./calcUtau(p);
    gp<<"plot 0 not lt -1,"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Axial',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Circumferential',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Radial'"<<endl;
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+1))) );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+2))) );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Radial profiles of averaged velocities", ""
    )));
    
  }
  
  // Mean reynolds stress profiles
  {
    Gnuplot gp;
    string chart_name="mean_Rstress";
    string chart_file_name=chart_name+".png";
    double fac_yp=Re_tau*2.0/D;
    double fac_Rp=1./pow(calcUtau(p),2);
    int c=cd["UPrime2Mean"].col;
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y+'; set ylabel '<R+>'; set grid; ";
    gp<<"set logscale x;";
    gp<<"set yrange [:"<<fac_Rp*max(data.col(c))<<"];";
    
    
    gp<<"plot 0 not lt -1,"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rxx (Axial)',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Ryy (Circumferential)',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rzz (Radial)'"<<endl;
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+3))) );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+5))) );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Radial profiles of averaged reynolds stresses", ""
    )));
    
  }

  const RadialTPCArray* tpcs=cm.get<RadialTPCArray>("tpc_interiorRadialTPCArray");
  if (!tpcs)
    throw insight::Exception("tpc FO array not found in case!");
  tpcs->evaluate(cm, executionPath(), results);
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  std::string pressure_contour_name="pressure_longi";
  std::string pressure_contour_filename=pressure_contour_name+".jpg";
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
      "eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
      "Show(eb)\n"
      "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
      "setCam([0,0,10], [0,0,0], [0,1,0])\n"
      "WriteImage('"+pressure_contour_filename+"')\n"
    )
  );
  results->insert(pressure_contour_name,
    std::auto_ptr<Image>(new Image
    (
    pressure_contour_filename, 
    "Contour of pressure (longitudinal section)", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    std::string velocity_contour_name="U"+c+"Contour";
    string velocity_contour_filename=velocity_contour_name+".jpg";
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
	"Show(eb)\n"
	"displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	"setCam([0,0,10], [0,0,0], [0,1,0])\n"
	"WriteImage('"+velocity_contour_filename+"')\n"
      )
    );
    results->insert(velocity_contour_name,
      std::auto_ptr<Image>(new Image
      (
      velocity_contour_filename, 
      "Contour of "+c+"-Velocity", ""
    )));
  }

  
  return results;
}




defineType(PipeCyclic);

PipeCyclic::PipeCyclic(const NoParameters& nop)
: PipeBase(nop)
{
}

void PipeCyclic::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  PipeBase::createMesh(cm, p);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeCyclic::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  double T=calcT(p);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  PipeBase::createCase(cm, p);
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(calcUbulk(p), 0, 0))
		));

}

void PipeCyclic::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "operation", Re_tau);
  
  /*
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(calcUbulk(p))+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  cm.executeCommand(executionPath(), "applyBoundaryLayer", list_of<string>("-ybl")(lexical_cast<string>(0.25)) );
  cm.executeCommand(executionPath(), "randomizeVelocity", list_of<string>(lexical_cast<string>(0.1*calcUbulk(p))) );
  */
  
  cm.executeCommand(executionPath(), "perturbU", 
		    list_of<string>
		    (lexical_cast<string>(Re_tau))
		    ("("+lexical_cast<string>(calcUbulk(p))+" 0 0)") 
		   );
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void PipeCyclic::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);
  double T=calcT(p);

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
  controlDict["endTime"] = (inittime+meantime+mean2time)*T;
}

addToFactoryTable(Analysis, PipeCyclic, NoParameters);




defineType(PipeInflow);

const char* PipeInflow::tpc_names_[] = 
  {
    "tpc0_inlet",
    "tpc1_intermediate1",
    "tpc2_intermediate2",
    "tpc3_intermediate3"
  };

const double PipeInflow::tpc_xlocs_[] = {0.0, 0.125, 0.25, 0.375};

PipeInflow::PipeInflow(const NoParameters& nop)
: PipeBase(nop)
{
}

void PipeInflow::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  PipeBase::createMesh(cm, p);
  //convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeInflow::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  double T=calcT(p);

  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new TurbulentVelocityInletBC(cm, cycl_in_, boundaryDict, TurbulentVelocityInletBC::Parameters()
    .set_velocity(vec3(calcUbulk(p), 0, 0))
    .set_turbulenceIntensity(0.05)
    .set_mixingLength(0.1*D)
  ));
  
  cm.insert(new PressureOutletBC(cm, cycl_out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
  ));
  
  PipeBase::createCase(cm, p);
  
  for (int i=0; i<ntpc_; i++)
  {
    cm.insert(new RadialTPCArray(cm, RadialTPCArray::Parameters()
      .set_name_prefix(tpc_names_[i])
      .set_R(0.5*D)
      .set_x(tpc_xlocs_[i]*L)
      .set_axSpan(0.5*L)
      .set_tanSpan(M_PI)
      .set_timeStart( (inittime+meantime)*T )
    ));
  }
  
}

ResultSetPtr PipeInflow::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  for (int i=0; i<ntpc_; i++)
  {
    const RadialTPCArray* tpcs=cm.get<RadialTPCArray>( string(tpc_names_[i])+"RadialTPCArray");
    if (!tpcs)
      throw insight::Exception("tpc FO array "+string(tpc_names_[i])+" not found in case!");
    tpcs->evaluate(cm, executionPath(), results);

    std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
    std::string pressure_contour_name="sliceAt_"+string(tpc_names_[i]);
    std::string pressure_contour_filename=pressure_contour_name+".jpg";
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
	"Show(eb)\n"
	"displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
	"setCam([0,0,10], [0,0,0], [0,1,0])\n"
	"WriteImage('"+pressure_contour_filename+"')\n"
      )
    );
    results->insert(pressure_contour_name,
      std::auto_ptr<Image>(new Image
      (
      pressure_contour_filename, 
      "Contour of pressure (longitudinal section)", ""
    )));
  }
    
  return results;
}

void PipeInflow::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(calcUbulk(p))+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  
  cm.get<TurbulentVelocityInletBC>(cycl_in_+"BC")->initInflowBC(executionPath());
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void PipeInflow::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);
  double T=calcT(p);

  OpenFOAMAnalysis::applyCustomOptions(cm, p, dicts);
  
  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["endTime"] = (inittime+meantime+mean2time)*T;
}

addToFactoryTable(Analysis, PipeInflow, NoParameters);

}
