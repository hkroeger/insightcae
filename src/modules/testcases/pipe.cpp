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
#include "refdata.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
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
  
  return Re_tau*((1./k)*log(Re_tau)+Cplus-3.04);
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

double PipeBase::UmaxByUbulk(double Retau)
{
  return 1 + 0.7 * Retau/PipeBase::Re(Retau); // Constant not given explictly in Schlichting, taken from Rotta, p. 190
}

PipeBase::PipeBase()
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
	    ("x",	new DoubleParameter(0.33, "Edge length of core block as fraction of diameter"))
	    ("fixbuf",	new BoolParameter(false, "fix cell layer size inside buffer layer"))
	    ("dzplus",	new DoubleParameter(15, "Dimensionless grid spacing in spanwise direction"))
	    ("dxplus",	new DoubleParameter(60, "Dimensionless grid spacing in axial direction"))
	    ("ypluswall", new DoubleParameter(0.5, "yPlus at the wall grid layer"))
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
      
      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("perturbU", 	new BoolParameter(true, "Whether to impose artifical perturbations on the initial velocity field"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Execution parameters"
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

void PipeBase::calcDerivedInputData()
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);

  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSDBL(p, "mesh", x);
  PSBOOL(p, "mesh", fixbuf);

  // Physics
  Re_=Re(Re_tau);
  Ubulk_=Re_/Re_tau;
  T_=L/Ubulk_;
  nu_=1./Re_tau;
  utau_=Re_tau*nu_/(0.5*D);

  Lc_ = x*D;
  nax_=int(L*Re_tau/dxplus);  
  nc_=int(M_PI*D*Re_tau/dzplus) /4;
  
  double rc=0.5*D-0.5*sqrt(2.*Lc_*Lc_); // radial distance core block edge => outer wall

  ywall_ = ypluswall*0.5*D/Re_tau;

  nrbuf_=0;
  if (fixbuf>0)
  {
    double ypbuf=30.;
    rbuf_=ypbuf/Re_tau;
    nrbuf_=std::max(1.0, rbuf_/ywall_);

  }
  
  bmd::GradingAnalyzer ga( ywall_, Lc_/double(nc_) );
  gradr_=ga.grad();
  cout<<gradr_<<endl;
  nr_=ga.calc_n(ywall_, rc-rbuf_);
  
  cout<<"Derived data:"<<endl
      <<"============================================="<<endl;
  cout<<"Reynolds number \tRe="<<Re_<<endl;
  cout<<"Bulk velocity \tUbulk="<<Ubulk_<<endl;
  cout<<"Flow-through time \tT="<<T_<<endl;
  cout<<"Viscosity \tnu="<<nu_<<endl;
  cout<<"Friction velocity \tutau="<<utau_<<endl;
  cout<<"Wall distance of first grid point \tywall="<<ywall_<<endl;
  cout<<"# cells circumferential \tnc="<<nc_<<endl;
  cout<<"# cells radial \tnr="<<nr_<<endl;
  cout<<"# grading vertical \tgradr="<<gradr_<<endl;
  cout<<"============================================="<<endl;
}



void PipeBase::createMesh
(
  OpenFOAMCase& cm
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  const ParameterSet& p=*parameters_;
  
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSBOOL(p, "mesh", fixbuf);

  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (12, 	vec3(0, 0.5*D, 0))
      (11, 	vec3(0, 0.5*D-rbuf_, 0))
      (10, 	vec3(0,  cos(0.5*al)*Lc_, 0.))
      (9, 	vec3(0,  1.2*0.5*Lc_, 0.))
      .convert_to_container<std::map<int, Point> >()
  ;
  arma::mat vL=vec3(L, 0, 0);
  arma::mat ax=vec3(1, 0, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  
  // core block
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
	nc_, nc_, nax_
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));
  }

  // radial blocks
  for (int i=0; i<4; i++)
  {
    arma::mat r0=rotMatrix(double(i+0.5)*al, ax);
    arma::mat r1=rotMatrix(double(i+1.5)*al, ax);

    {    
      Block& bl = bmd->addBlock
      (
	new Block(P_8(
	    r1*pts[10], r0*pts[10], r0*pts[11], r1*pts[11],
	    (r1*pts[10])+vL, (r0*pts[10])+vL, (r0*pts[11])+vL, (r1*pts[11])+vL
	  ),
	  nc_, nr_, nax_,
	  list_of<double>(1)(1./gradr_)(1)
	)
      );
      cycl_in.addFace(bl.face("0321"));
      cycl_out.addFace(bl.face("4567"));
    }

    if (fixbuf)
    {    
      Block& bl = bmd->addBlock
      (
	new Block(P_8(
	    r1*pts[11], r0*pts[11], r0*pts[12], r1*pts[12],
	    (r1*pts[11])+vL, (r0*pts[11])+vL, (r0*pts[12])+vL, (r1*pts[12])+vL
	  ),
	  nc_, nrbuf_, nax_,
	  list_of<double>(1)(1)(1)
	)
      );
      cycl_in.addFace(bl.face("0321"));
      cycl_out.addFace(bl.face("4567"));
    }

    arma::mat rmid=rotMatrix(double(i+1)*al, ax);
    bmd->addEdge(new ArcEdge(r1*pts[12], r0*pts[12], rmid*pts[12]));
    bmd->addEdge(new ArcEdge((r1*pts[12])+vL, (r0*pts[12])+vL, (rmid*pts[12])+vL));

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
  OpenFOAMCase& cm
)
{
  const ParameterSet& p=*parameters_;
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  
  cm.insert(new cuttingPlane(cm, cuttingPlane::Parameters()
    .set_fields(list_of<string>("p")("U")("UMean")("UPrime2Mean"))
    .set_basePoint(vec3(0,1e-6,1e-6))
    .set_normal(vec3(0,0,1))
    .set_name("plane")
  ));
  
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(inittime*T_)
    .set_name("zzzaveraging") // shall be last FO in list
  ));
  
  cm.insert(new RadialTPCArray(cm, typename RadialTPCArray::Parameters()
    .set_name_prefix("tpc_interior")
    .set_R(0.5*D)
      .set_p0(vec3(0, 0, 0.5*L)) // in cyl coord sys
      .set_e_ax(vec3(0,0,1))
      .set_e_rad(vec3(1,0,0))
      .set_e_tan(vec3(0,1,0))
//     .set_x(0.5*L)
    .set_axSpan(0.5*L)
    .set_tanSpan(M_PI)
    .set_timeStart( (inittime+meantime)*T_ )
  ));
  
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu_) ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

}

void PipeBase::evaluateAtSection(
  OpenFOAMCase& cm, 
  ResultSetPtr results, double x, int i
)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ostringstream sns; sns<<"section_x"<<x;
  string title=sns.str();
  replace_all(title, ".", "_");
    
  boost::ptr_vector<sampleOps::set> sets;
  
  sets.push_back(new sampleOps::circumferentialAveragedUniformLine(sampleOps::circumferentialAveragedUniformLine::Parameters()
    .set_name("section"+lexical_cast<string>(i))
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
    
  arma::mat refdata_umean180=refdatalib.getProfile("K_Pipe", "180/uzmean_vs_yp");
  arma::mat refdata_vmean180=refdatalib.getProfile("K_Pipe", "180/urmean_vs_yp");
  arma::mat refdata_wmean180=refdatalib.getProfile("K_Pipe", "180/uphimean_vs_yp");
    
  // Mean velocity profiles
  {
    int c=cd["UMean"].col;
    double fac_yp=Re_tau*2.0/D;
    double fac_Up=1./utau_;

    addPlot
    (
      results, executionPath(), "chartMeanVelocity_"+title,
      "$y^+$", "$\\langle U^+ \\rangle$",
      list_of<PlotCurve>
	(PlotCurve( arma::mat(join_rows(Re_tau-fac_yp*data.col(0), fac_Up*data.col(c))), "Up", "w l lt 1 lc 1 lw 3 t 'Axial'"))
	(PlotCurve( arma::mat(join_rows(Re_tau-fac_yp*data.col(0), fac_Up*data.col(c+1))), "Vp", "w l lt 1 lc 2 lw 3 t 'Radial'" ))
	(PlotCurve( arma::mat(join_rows(Re_tau-fac_yp*data.col(0), fac_Up*data.col(c+2))), "Wp", "w l lt 1 lc 3 lw 3 t 'Circumferential'" ))
	(PlotCurve( refdata_umean180, "Uref", "w l lt 2 lc 1 t '$U_{ref}$ (K\\_Pipe)'" ))
	(PlotCurve( refdata_vmean180, "Vref", "w l lt 2 lc 2 t '$V_{ref}$ (K\\_Pipe)'" ))
	(PlotCurve( refdata_wmean180, "Wref", "w l lt 2 lc 3 t '$W_{ref}$ (K\\_Pipe)'" )),
      "Radial profiles of averaged velocities",
      "set logscale x;"
    );
    
//     Gnuplot gp;
//     string chart_name="chartMeanVelocity_"+title;
//     string chart_file_name=chart_name+".png";
//     
//     gp<<"set terminal png; set output '"<<chart_file_name<<"';";
//     gp<<"set xlabel 'y+'; set ylabel '<U+>'; set grid; ";
//     gp<<"set logscale x;";
//     
//     gp<<"plot 0 not lt -1,"
// 	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Axial',"
// 	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Circumferential',"
// 	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Radial'"<<endl;
//     gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
//     gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+1))) );
//     gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+2))) );
// 
//     results->insert(chart_name,
//       std::auto_ptr<Image>(new Image
//       (
//       chart_file_name, 
//       "Radial profiles of averaged velocities", ""
//     )));
    
  }
  
  arma::mat refdata_Ruu=refdatalib.getProfile("K_Pipe", "180/Rzz_vs_yp");
  arma::mat refdata_Rvv=refdatalib.getProfile("K_Pipe", "180/Rrr_vs_yp");
  arma::mat refdata_Rww=refdatalib.getProfile("K_Pipe", "180/Rphiphi_vs_yp");
  
  arma::mat refdata_K=refdata_Ruu;
  refdata_K.col(1)+=Interpolator(refdata_Rvv)(refdata_Ruu.col(0));
  refdata_K.col(1)+=Interpolator(refdata_Rww)(refdata_Ruu.col(0));
  refdata_K.col(1)*=0.5;
  refdata_K.col(0)/=180.0;

  // Mean reynolds stress profiles
  {
    int c=cd["UPrime2Mean"].col;
    double fac_yp=Re_tau*2.0/D;
    double fac_Rp=1./pow(utau_,2);

    addPlot
    (
      results, executionPath(), "chartMeanRstress_"+title,
      "$y^+$", "$\\langle R^+ \\rangle$",
      list_of<PlotCurve>
	(PlotCurve( arma::mat(join_rows(Re_tau-fac_yp*data.col(0), fac_Rp*data.col(c))),   "Ruu", "w l lt 1 lc 1 lw 4 t '$R_{uu}$ (axial)'"  ))
	(PlotCurve( arma::mat(join_rows(Re_tau-fac_yp*data.col(0), fac_Rp*data.col(c+3))), "Rvv", "w l lt 1 lc 2 lw 4 t '$R_{vv}$ (radial)'" ))
	(PlotCurve( arma::mat(join_rows(Re_tau-fac_yp*data.col(0), fac_Rp*data.col(c+5))), "Rww", "w l lt 1 lc 3 lw 4 t '$R_{ww}$ (circumf.)'" ))
	(PlotCurve( refdata_Ruu, "Ruuref", "w l lt 2 lc 1 t '$R_{uu,ref}$ (K\\_Pipe, $Re_{\\tau}=180$)'"  ))
	(PlotCurve( refdata_Rvv, "Rvvref", "w l lt 2 lc 2 t '$R_{vv,ref}$ (K\\_Pipe, $Re_{\\tau}=180$)'" ))
	(PlotCurve( refdata_Rww, "Rwwref", "w l lt 2 lc 3 t '$R_{ww,ref}$ (K\\_Pipe, $Re_{\\tau}=180$)'" ))
	,
      "Radial profiles of averaged reynolds stresses",
      "set yrange [:"+lexical_cast<string>(fac_Rp*max(data.col(c)))+"];"
    );
    
//     Gnuplot gp;
//     string chart_name="chartMeanRstress_"+title;
//     string chart_file_name=chart_name+".png";
//     
//     gp<<"set terminal png; set output '"<<chart_file_name<<"';";
//     gp<<"set xlabel 'y+'; set ylabel '<R+>'; set grid; ";
//     //gp<<"set logscale x;";
//     gp<<"set yrange [:"<<fac_Rp*max(data.col(c))<<"];";
//     
//     
//     gp<<"plot 0 not lt -1,"
// 	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rxx (Axial)',"
// 	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Ryy (Circumferential)',"
// 	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rzz (Radial)'"<<endl;
//     gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
//     gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+3))) );
//     gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+5))) );
// 
//     results->insert(chart_name,
//       std::auto_ptr<Image>(new Image
//       (
//       chart_file_name, 
//       "Radial profiles of averaged reynolds stresses", ""
//     )));
//     
  }

  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  std::string pressure_contour_name="contourPressure_ax_"+title;
  std::string pressure_contour_filename=pressure_contour_name+".png";
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
      "eb = planarSlice(cbi, ["+lexical_cast<string>(x)+",0,1e-6], [1,0,0])\n"
      "Show(eb)\n"
      "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
      "setCam([-10,0,0], [0,0,0], [0,1,0])\n"
      "WriteImage('"+pressure_contour_filename+"')\n"
    )
  );
  results->insert(pressure_contour_name,
    std::auto_ptr<Image>(new Image
    (
    executionPath(), pressure_contour_filename, 
    "Contour of pressure (axial section)", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    std::string velocity_contour_name="contourU"+c+"_ax_"+title;
    string velocity_contour_filename=velocity_contour_name+".png";
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, ["+lexical_cast<string>(x)+",0,1e-6], [1,0,0])\n"
	"Show(eb)\n"
	"displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	"setCam([-10,0,0], [0,0,0], [0,1,0])\n"
	"WriteImage('"+velocity_contour_filename+"')\n"
      )
    );
    results->insert(velocity_contour_name,
      std::auto_ptr<Image>(new Image
      (
      executionPath(), velocity_contour_filename, 
      "Contour of "+c+"-Velocity (axial section)", ""
    )));
  }
}

  
ResultSetPtr PipeBase::evaluateResults(OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm);
/*  
  boost::ptr_vector<sampleOps::set> sets;*/
  
  //double x=L*0.5;
  evaluateAtSection(cm, results, 0.5*L, 0);

  const RadialTPCArray* tpcs=cm.get<RadialTPCArray>("tpc_interiorTPCArray");
  if (!tpcs)
    throw insight::Exception("tpc FO array not found in case!");
  tpcs->evaluate(cm, executionPath(), results,
    "two-point correlation of velocity at different radii at x/L=0.5"
  );
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  std::string pressure_contour_name="contourPressure_longitudinal";
  std::string pressure_contour_filename=pressure_contour_name+".png";
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
    executionPath(), pressure_contour_filename, 
    "Contour of pressure (longitudinal section)", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    std::string velocity_contour_name="contourU"+c+"_longitudinal";
    string velocity_contour_filename=velocity_contour_name+".png";
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
      executionPath(), velocity_contour_filename, 
      "Contour of "+c+"-Velocity (longitudinal section)", ""
    )));
  }

  
  return results;
}




defineType(PipeCyclic);

PipeCyclic::PipeCyclic()
: PipeBase()
{
}

void PipeCyclic::createMesh
(
  OpenFOAMCase& cm
)
{  
  PipeBase::createMesh(cm);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeCyclic::createCase
(
  OpenFOAMCase& cm
)
{  
  const ParameterSet& p=*parameters_;
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
        .set_hasCyclics(true)
  ));
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  PipeBase::createCase(cm);
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(Ubulk_, 0, 0))
		));

}

void PipeCyclic::applyCustomPreprocessing(OpenFOAMCase& cm)
{
  if (parameters().getBool("run/perturbU"))
  {
    PSDBL(parameters(), "operation", Re_tau);
    
    cm.executeCommand(executionPath(), "perturbU", 
		      list_of<string>
		      (lexical_cast<string>(Re_tau))
		      ("("+lexical_cast<string>(Ubulk_)+" 0 0)") 
		    );
  }
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm);
}

void PipeCyclic::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  OpenFOAMAnalysis::applyCustomOptions(cm, dicts);
  
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

addToFactoryTable(Analysis, PipeCyclic);




defineType(PipeInflow);

const char* PipeInflow::tpc_names_[] = 
  {
    "tpc0_inlet",
    "tpc1_intermediate1",
    "tpc2_intermediate2",
    "tpc3_intermediate3"
  };

const double PipeInflow::tpc_xlocs_[] = {0.0, 0.125, 0.25, 0.375};

PipeInflow::PipeInflow()
: PipeBase()
{
}

ParameterSet PipeInflow::defaultParameters() const
{
  ParameterSet p(PipeBase::defaultParameters());

  std::auto_ptr<SubsetParameter> inflowparams(new SubsetParameter(TurbulentVelocityInletBC::Parameters::makeDefault(), "Inflow BC"));
  
//   (*inflowparams)().extend
//   (
//       boost::assign::list_of<ParameterSet::SingleEntry>
//       ("umean", FieldData::defaultParameter(vec3(1,0,0)))
//       .convert_to_container<ParameterSet::EntryList>()
//   );
//   
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    ("inflow", inflowparams.release())
    .convert_to_container<ParameterSet::EntryList>()
  );
    
  return p;
}

void PipeInflow::createMesh
(
  OpenFOAMCase& cm
)
{  
  PipeBase::createMesh(cm);
  //convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeInflow::createCase
(
  OpenFOAMCase& cm
)
{  
  const ParameterSet& p=*parameters_;
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);

  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
  ));

  cm.insert(new TurbulentVelocityInletBC( cm, cycl_in_, boundaryDict, p.getSubset("inflow") ));
  
  cm.insert(new PressureOutletBC(cm, cycl_out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
    .set_fixMeanValue(true)
  ));
  
  PipeBase::createCase(cm);
  
  for (int i=0; i<ntpc_; i++)
  {
    cm.insert(new RadialTPCArray(cm, RadialTPCArray::Parameters()
      .set_name_prefix(tpc_names_[i])
      .set_R(0.5*D)
//       .set_x(tpc_xlocs_[i]*L)
      .set_p0(vec3(0, 0, tpc_xlocs_[i]*L)) // in cyl coord sys
      .set_e_ax(vec3(0,0,1))
      .set_e_rad(vec3(1,0,0))
      .set_e_tan(vec3(0,1,0))
      .set_axSpan(0.5*L)
      .set_tanSpan(M_PI)
      .set_timeStart( (inittime+meantime)*T_ )
    ));
  }
  
}

ResultSetPtr PipeInflow::evaluateResults(OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm);
  for (int i=0; i<ntpc_; i++)
  {
    evaluateAtSection(cm, results, (tpc_xlocs_[i]+1e-6)*L, i+1);
    
    const RadialTPCArray* tpcs=cm.get<RadialTPCArray>( string(tpc_names_[i])+"TPCArray");
    if (!tpcs)
      throw insight::Exception("tpc FO array "+string(tpc_names_[i])+" not found in case!");
    tpcs->evaluate(cm, executionPath(), results,
      "two-point correlation of velocity at different radii at x/L="+lexical_cast<string>(tpc_xlocs_[i]+1e-6)
    );
  }
  
  // ============= Longitudinal profile of Velocity an RMS ================
  int nr=10;
  for (int i=0; i<nr; i++)
  {
    double r0=0.1, r1=0.997;
    double r=r0+(r1-r0)*double(i)/double(nr-1);
    
    ostringstream sns; sns<<"longitudinal_r"<<r;
    string title=sns.str();
    replace_all(title, ".", "_");

    boost::ptr_vector<sampleOps::set> sets;
    
    sets.push_back(new sampleOps::circumferentialAveragedUniformLine(sampleOps::circumferentialAveragedUniformLine::Parameters()
      .set_name("longitudinal"+lexical_cast<string>(i))
      .set_start( vec3(0.001*L, 0, r*0.5*D))
      .set_end(   vec3(0.999*L, 0, r*0.5*D))
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
      string chart_name="chartMeanVelocity_"+title;
      string chart_file_name=chart_name+".png";
      
      gp<<"set terminal png; set output '"<<chart_file_name<<"';";
      gp<<"set xlabel 'x+'; set ylabel '<U+>'; set grid; ";
      //gp<<"set logscale x;";
      
      int c=cd["UMean"].col;
      
      double fac_yp=Re_tau*2.0/D;
      double fac_Up=1./utau_;
      gp<<"plot 0 not lt -1,"
	  " '-' u ($1*"<<fac_yp<<"):($2*"<<fac_Up<<") w l t 'Axial',"
	  " '-' u ($1*"<<fac_yp<<"):($2*"<<fac_Up<<") w l t 'Circumferential',"
	  " '-' u ($1*"<<fac_yp<<"):($2*"<<fac_Up<<") w l t 'Radial'"<<endl;
      gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
      gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+1))) );
      gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+2))) );

      results->insert(chart_name,
	std::auto_ptr<Image>(new Image
	(
	executionPath(), chart_file_name, 
	"Longitudinal profiles of averaged velocities", ""
      )));
      
    }
    
    // Mean reynolds stress profiles
    {
      Gnuplot gp;
      string chart_name="chartMeanRstress_"+title;
      string chart_file_name=chart_name+".png";
      double fac_yp=Re_tau*2.0/D;
      double fac_Rp=1./pow(utau_,2);
      int c=cd["UPrime2Mean"].col;
      
      gp<<"set terminal png; set output '"<<chart_file_name<<"';";
      gp<<"set xlabel 'x+'; set ylabel '<R+>'; set grid; ";
      //gp<<"set logscale x;";
      gp<<"set yrange [:"<<fac_Rp*max(data.col(c))<<"];";
      
      
      gp<<"plot 0 not lt -1,"
	  " '-' u ($1*"<<fac_yp<<"):($2*"<<fac_Rp<<") w l t 'Rxx (Axial)',"
	  " '-' u ($1*"<<fac_yp<<"):($2*"<<fac_Rp<<") w l t 'Ryy (Circumferential)',"
	  " '-' u ($1*"<<fac_yp<<"):($2*"<<fac_Rp<<") w l t 'Rzz (Radial)'"<<endl;
      gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
      gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+3))) );
      gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+5))) );

      results->insert(chart_name,
	std::auto_ptr<Image>(new Image
	(
	executionPath(), chart_file_name, 
	"Longitudinal profiles of averaged reynolds stresses", ""
      )));
      
    }
  }
    
  return results;
}

void PipeInflow::applyCustomPreprocessing(OpenFOAMCase& cm)
{
  
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(Ubulk_)+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  
//   cm.get<TurbulentVelocityInletBC>(cycl_in_+"BC")->initInflowBC(executionPath(), p.getSubset("inflow"));
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm);
}

void PipeInflow::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  OpenFOAMAnalysis::applyCustomOptions(cm, dicts);
  
  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["endTime"] = (inittime+meantime+mean2time)*T_;
}

addToFactoryTable(Analysis, PipeInflow);

}
