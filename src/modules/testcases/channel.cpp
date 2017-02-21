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
#include "openfoam/basiccaseelements.h"
#include "refdata.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"
#include "boost/format.hpp"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{




defineType(ChannelBase);

double ChannelBase::Re(double Re_tau)
{
  double k=0.41;
  double Cplus=5.0;
  
  return Re_tau*( (1./k)*log(Re_tau)+Cplus-1.7 );
}


double ChannelBase::Retau(double Re)
{
  struct Obj: public Objective1D
  {
    double Re;
    virtual double operator()(double x) const { return Re - ChannelBase::Re(x); }
  } obj;
  obj.Re=Re;
  return nonlinearSolve1D(obj, 1e-3*Re, Re);
}

double ChannelBase::UmaxByUbulk(double Retau)
{
  return 1 + 2.64 * Retau/ChannelBase::Re(Retau);
}


ChannelBase::ChannelBase(const ParameterSet& ps, const boost::filesystem::path& exepath)
: OpenFOAMAnalysis
  (
    "Channel Flow Test Case",
    "Rectangular domain with cyclic BCs on axial ends",
    ps, exepath
  ),
  cycl_in_("cycl_half0"),
  cycl_out_("cycl_half1")
{
}

ChannelBase::~ChannelBase()
{

}

ParameterSet ChannelBase::defaultParameters()
{
  ParameterSet p(Parameters::makeDefault());
  p.merge(OpenFOAMAnalysis::defaultParameters());
  return p;
}

std::string ChannelBase::cyclPrefix() const
{
  boost::smatch m;
  boost::regex_search(cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  return namePrefix;
}

void ChannelBase::calcDerivedInputData()
{
  Parameters p(parameters_);
  
  PSDBL(parameters(), "geometry", H);
  PSDBL(parameters(), "geometry", B);
  PSDBL(parameters(), "geometry", L);
  PSDBL(parameters(), "operation", Re_tau);

  PSDBL(parameters(), "mesh", ypluswall);
  PSDBL(parameters(), "mesh", layerratio);
  PSDBL(parameters(), "mesh", dxplus);
  PSDBL(parameters(), "mesh", dzplus);
  PSINT(parameters(), "mesh", nh);
  PSINT(parameters(), "mesh", nl);
  PSBOOL(parameters(), "mesh", fixbuf);
  
  // Physics
  Re_=Re(Re_tau);
  Ubulk_=Re_/Re_tau;
  T_=L/Ubulk_;
  nu_=1./Re_tau;
  utau_=Re_tau*nu_/(0.5*H);
  ywall_ = ypluswall/Re_tau;
  
  // grid
  //double Delta=L/double(nax);
  nax_=std::max(1, int(L*Re_tau/dxplus));
  
  if (p.mesh.twod)
    nb_=1;
  else
    nb_=std::max(1, int(B*Re_tau/dzplus));

  hbuf_=0.0;  
  nh_=std::max(1, nh/2);
  
  
  
  nhbuf_=0;
  gradl_=1.;
  if (fixbuf>0)
  {
    if (nh_-nhbuf_<=1)
      throw insight::Exception("Cannot fix cell height inside buffer layer: too few cells in vertical direction allowed! (min "+lexical_cast<string>(nhbuf_+1)+")");

    nhbuf_=nl;
    
    gradl_=pow(layerratio, nl-1);
    reportIntermediateParameter("gradl", gradl_, "near-wall layer block grading");
    
    hbuf_=bmd::GradingAnalyzer(gradl_).calc_L(ywall_, nl);
    hbuf_=std::min(0.49*H, hbuf_);
    reportIntermediateParameter("hbuf", hbuf_, "near-wall layer block height (clipped to 0.9*H)");

//     double ypbuf=30.;
//     hbuf_=ypbuf/Re_tau;
//     nhbuf_=std::max(1.0, hbuf_/ywall_);
      //ywall_=hbuf_/double(nhbuf);
    
  }

  gradh_=bmd::GradingAnalyzer
  (
    bmd::GradingAnalyzer(gradl_).calc_delta1(ywall_), 
    0.5*H-hbuf_, 
    nh_-nhbuf_
  ).grad();
  //nh_=max(1, bmd::GradingAnalyzer(gradh_).calc_n(ywall_, H/2.));
  
  if (const Parameters::run_type::regime_steady_type *steady 
	= boost::get<Parameters::run_type::regime_steady_type>(&p.run.regime))
  {
    end_=steady->iter;
    avgStart_=0.98*end_;
    avg2Start_=end_;
  } 
  else if (const Parameters::run_type::regime_unsteady_type *unsteady 
	= boost::get<Parameters::run_type::regime_unsteady_type>(&p.run.regime))
  {
    avgStart_=unsteady->inittime*T_;
    avg2Start_=avgStart_+unsteady->meantime*T_;
    end_=avg2Start_+unsteady->mean2time*T_;
  }

  cout<<"Derived data:"<<endl
      <<"============================================="<<endl;
  cout<<"Reynolds number \tRe="<<Re_<<endl;
  cout<<"Bulk velocity \tUbulk="<<Ubulk_<<endl;
  cout<<"Flow-through time \tT="<<T_<<endl;
  cout<<"Viscosity \tnu="<<nu_<<endl;
  cout<<"Height of buffer layer\thbuf="<<hbuf_<<endl;
  cout<<"No cells in buffer layer\tnhbuf="<<nhbuf_<<endl;
  cout<<"Friction velocity \tutau="<<utau_<<endl;
  cout<<"Wall distance of first grid point \tywall="<<ywall_<<endl;
  cout<<"# cells axial \tnax="<<nax_<<endl;
  cout<<"# cells spanwise \tnb="<<nb_<<endl;
  cout<<"# grading vertical \tgradh="<<gradh_<<endl;
  cout<<"============================================="<<endl;
}

void ChannelBase::createMesh
(
  OpenFOAMCase& cm
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  Parameters p(parameters_);
  
  PSDBL(parameters(), "geometry", H);
  PSDBL(parameters(), "geometry", B);
  PSDBL(parameters(), "geometry", L);
  PSDBL(parameters(), "operation", Re_tau);
  
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
      .convert_to_container<std::map<int, Point> >()
  ;
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  string side_type="cyclic";
  if (p.mesh.twod) side_type="empty";
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
	  list_of<double>(1.)(1.)(gradl_)
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
	  list_of<double>(1.)(1.)(1./gradl_)
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


void ChannelBase::createCase
(
  OpenFOAMCase& cm
)
{
  Parameters p(parameters_);

  // create local variables from ParameterSet
  PSDBL(parameters(), "geometry", H);
  PSDBL(parameters(), "geometry", B);
  PSDBL(parameters(), "geometry", L);
  PSDBL(parameters(), "operation", Re_tau);
  PSINT(parameters(), "fluid", turbulenceModel);
    
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

//   cm.insert( new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
//   ) );
  
  if (Parameters::run_type::regime_steady_type *steady 
	= boost::get<Parameters::run_type::regime_steady_type>(&p.run.regime))
  {
    cm.insert(new simpleFoamNumerics(cm, simpleFoamNumerics::Parameters()
      .set_checkResiduals(false) // don't stop earlier since averaging should be completed
      .set_hasCyclics(true)
      .set_Uinternal(vec3(Ubulk_,0,0))
      .set_decompositionMethod(FVNumerics::Parameters::decompositionMethod_type::hierarchical)
      .set_endTime(end_)
      .set_decompWeights(vec3(2,1,0))
      .set_np(p.OpenFOAMAnalysis::Parameters::run.np)
    ));
  } 
  else if (Parameters::run_type::regime_unsteady_type *unsteady 
	= boost::get<Parameters::run_type::regime_unsteady_type>(&p.run.regime))
  {
    cm.insert( new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
      .set_LESfilteredConvection(p.run.filteredconvection)
      .set_maxDeltaT(0.25*T_)
      .set_Uinternal(vec3(Ubulk_,0,0))
      .set_hasCyclics(true)
      .set_writeControl(FVNumerics::Parameters::writeControl_type::adjustableRunTime)
      .set_writeInterval(0.25*T_)
      .set_endTime( end_ )
      .set_writeFormat(FVNumerics::Parameters::writeFormat_type::ascii)
      .set_decompositionMethod(FVNumerics::Parameters::decompositionMethod_type::simple)
      .set_deltaT( double(L/nax_)/Ubulk_ ) // Co=1
      .set_decompWeights(vec3(2,1,0))
      .set_np(p.OpenFOAMAnalysis::Parameters::run.np)
    ));
  }
  
  cm.insert(new extendedForces(cm, extendedForces::Parameters()
    .set_patches( list_of<string>("walls") )
  ));
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_fields(list_of<std::string>("p")("U")("pressureForce")("viscousForce"))
    .set_timeStart(avgStart_)
    .set_name("zzzaveraging") // shall be last FO in list
  ));
  
  if (p.run.eval2)
  {
    cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
      .set_R(0.5*H)
//       .set_x(0.0) // middle x==0!
//       .set_z(-0.49*B)
      .set_p0(vec3(0., 0., -0.49*B))
      .set_axSpan(0.5*L)
      .set_tanSpan(0.45*B)
      .set_name("tpc_interior")
      .set_timeStart( avg2Start_ )
    ));
  }

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu_) ));
  if (p.mesh.twod)
    cm.insert(new SimpleBC(cm, "cycl_side", boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, "cycl_side", boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters().set_roughness_z0(p.operation.y0));
  
  insertTurbulenceModel(cm, parameters().get<SelectionParameter>("fluid/turbulenceModel").selection());

}

void ChannelBase::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  Parameters p(parameters_);
  
  OpenFOAMAnalysis::applyCustomOptions(cm, dicts);
  
  if (p.mesh.twod) 
  {
    OFDictData::dict& fvSolution=dicts->lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers["p"]=stdSymmSolverSetup(1e-7, 0.01);
  }
//   OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
//   controlDict["maxDeltaT"]=0.5*T_;
}

void ChannelBase::evaluateAtSection(
  OpenFOAMCase& cm, 
  ResultSetPtr results, double x, int i,
  Ordering& o,
  bool includeRefDataInCharts,
  bool includeAllComponentsInCharts
)
{
  Parameters p(parameters_);

  PSDBL(parameters(), "geometry", B);
  PSDBL(parameters(), "geometry", H);
  PSDBL(parameters(), "geometry", L);
  PSDBL(parameters(), "operation", Re_tau);
    
  
  double xByH= (x/L + 0.5)*L/H;
  bool isfirstslice=false;
  if (xByH<=1e-3) isfirstslice=true;

  boost::shared_ptr<ResultSection> section
  (
    new ResultSection
    (
      str(format("Section at x/H=%.2f")%xByH), 
      ""
    )
  );
  Ordering so;
  
  string title="section__xByH_" + str(format("%04.2f") % xByH);
  replace_all(title, ".", "_");
    
  boost::ptr_vector<sampleOps::set> sets;
  
  double delta_yp1=1./Re_tau;   
  
  double
    miny=delta_yp1,
    maxy=0.5*H-delta_yp1;
    
  arma::mat pts = (miny + (maxy-miny)*cos(0.5*M_PI*(pow( linspace(0., 1., 101)-1.0, 2 )))) * vec3(0,1,0).t();
 
  pts.col(0)+=x;
  pts.col(2)+=-0.49*B;
 
  sets.push_back(new sampleOps::linearAveragedPolyLine(sampleOps::linearAveragedPolyLine::Parameters()
    .set_name("radial")
    .set_points( pts )
    .set_dir1(vec3(1,0,0))
    .set_dir2(vec3(0,0,0.98*B))
    .set_nd1(1)
    .set_nd2(n_hom_avg)
  ));
  sample(cm, executionPath(),
    
  list_of<std::string>(UMeanName_)("UPrime2Mean")("R")("k")("omega")("epsilon")("nut"),
     sets
  );    
      
  sampleOps::ColumnDescription cd;
  arma::mat data = dynamic_cast<sampleOps::linearAveragedPolyLine*>(&sets[0])
    ->readSamples(cm, executionPath(), &cd);
    
  // turn curve length along sample line into y-coordinate (add initial offset miny)
  data.col(0)+=miny;

  arma::mat refdata_umean180=refdatalib.getProfile("MKM_Channel", "180/umean_vs_yp");
  arma::mat refdata_wmean180=refdatalib.getProfile("MKM_Channel", "180/wmean_vs_yp");
  arma::mat refdata_umean395=refdatalib.getProfile("MKM_Channel", "395/umean_vs_yp");
  arma::mat refdata_wmean395=refdatalib.getProfile("MKM_Channel", "395/wmean_vs_yp");
  arma::mat refdata_umean590=refdatalib.getProfile("MKM_Channel", "590/umean_vs_yp");
  arma::mat refdata_wmean590=refdatalib.getProfile("MKM_Channel", "590/wmean_vs_yp");

  // Mean velocity profiles
  {
    int c=cd[UMeanName_].col;
    
    arma::mat axial(join_rows(Re_tau-Re_tau*data.col(0), data.col(c)));
    arma::mat wallnormal(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+1)));
    arma::mat spanwise(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+2)));
    
    axial.save( (executionPath()/("umeanaxial_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    wallnormal.save( (executionPath()/("umeanwallnormal_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    spanwise.save( (executionPath()/("umeanspanwise_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    
    std::vector<PlotCurve> plotcurves = list_of
      (PlotCurve(axial, 		"U", "w l lt 1 lc -1 lw 2 t '$U^+$'"))
      (PlotCurve(wallnormal, 	"V", "w l lt 1 lc 1 lw 2 t '$V^+$'"))
      (PlotCurve(spanwise, 		"W", "w l lt 1 lc 3 lw 2 t '$W^+$'"))
      ;
      
    if (includeRefDataInCharts)
    {
      std::vector<PlotCurve> pc = list_of
        (PlotCurve(refdata_umean180,	"UMKM180", "w l lt 2 lc -1 t '$U_{ref}^+(Re_{\\tau}=180)$'"))
        (PlotCurve(refdata_wmean180, 	"WMKM180", "w l lt 2 lc 3 t '$W_{ref}^+(Re_{\\tau}=180)$'"))
        (PlotCurve(refdata_umean395, 	"UMKM395", "w l lt 4 lc -1 t '$U_{ref}^+(Re_{\\tau}=395)$'"))
        (PlotCurve(refdata_wmean395, 	"WMKM395", "w l lt 4 lc 3 t '$W_{ref}^+(Re_{\\tau}=395)$'"))
        (PlotCurve(refdata_umean590, 	"UMKM590", "w l lt 3 lc -1 t '$U_{ref}^+(Re_{\\tau}=590)$'"))
        (PlotCurve(refdata_wmean590, 	"WMKM590", "w l lt 3 lc 3 t '$W_{ref}^+(Re_{\\tau}=590)$'"))
      ;
      plotcurves.insert(plotcurves.end(), pc.begin(), pc.end());
    }
      
    addPlot
    (
      section, executionPath(), "chartMeanVelocity_"+title,
      "$y^+$", "$\\langle U^+ \\rangle$",
      plotcurves,
      "Wall normal profiles of averaged velocities at x/H=" + str(format("%g")%xByH),
      "set logscale x; set key top left"
    ) 
    .setOrder(so.next());
    
  }
  
  // L profiles from k/omega
  if ((cd.find("k")!=cd.end()) && (cd.find("omega")!=cd.end()))
  {    
    arma::mat k=data.col(cd["k"].col);
    arma::mat omega=data.col(cd["omega"].col);
    
    arma::mat ydelta=1.0-(data.col(0)+delta_yp1)/(0.5*H); //Re_tau-Re_tau*data.col(0);
    arma::mat Lt1=(2./H)*sqrt(k)/(0.09*omega);
    arma::mat Lt2=ydelta*0.5*H*0.41;
    //arma::mat Lt=arma::min(Lt1, Lt2);
    arma::mat Lt=Lt1;
    for (int i=0; i<Lt2.n_rows; i++) Lt(i)=min(Lt(i), Lt2(i));
    arma::mat Ltp(join_rows(ydelta, Lt));
    Ltp.save( (executionPath()/("LdeltaRANS_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    
    struct cfm : public RegressionModel
    {
      double c0, c1, c2, c3;
        virtual int numP() const { return 4; }
	virtual void setParameters(const double* params)
	{
	  c0=params[0];
	  c1=params[1];
	  c2=params[2];
	  c3=params[3];
	}
	virtual void setInitialValues(double* params) const
	{
	  params[0]=1.0;
	  params[1]=-1.0;
	  params[2]=0.1;
	  params[3]=0.1;
	}

	virtual arma::mat evaluateObjective(const arma::mat& x) const
	{
	  return c0*pow(x, c2) + c1*pow(x, c3);
	}
    } m;
    nonlinearRegression(Lt, ydelta, m);
    arma::mat yfit=m.evaluateObjective(ydelta);
    
    addPlot
    (
      section, executionPath(), "chartTurbulentLengthScale_"+title,
      "$y_{\\delta}$", "$ \\langle L_{\\delta_{RANS}} \\rangle$",
      list_of
       (PlotCurve(arma::mat(join_rows(ydelta, Lt1)), "cfdkO", "w l lt 2 lc 1 lw 1 t 'CFD (from k and omega)'"))
       (PlotCurve(arma::mat(join_rows(ydelta, Lt2)), "Lmix", "w l lt 3 lc 1 lw 1 t 'Mixing length limit'"))
       (PlotCurve(Ltp, "cfd", "w l lt 1 lc 1 lw 2 t 'CFD'"))
       (PlotCurve(arma::mat(join_rows(ydelta, yfit)), "fit", "w l lt 2 lc 2 lw 2 t 'Fit'"))
       ,
      "Wall normal profile of turbulent length scale at $x/H=" + str(format("%g")%xByH) + "$. Fit: $"
		    + 	    str(format("%.5g") % m.c0)+" y_{\\delta}^{"+str(format("%.5g") % m.c2)+"}"
		    +" + ("+str(format("%.5g") % m.c1)+" y_{\\delta}^{"+str(format("%.5g") % m.c3)+"})$",
      "set key top left"
    )
    .setOrder(so.next());

    section->insert
    (
     "regressionCoefficientsTubulentLengthScale_"+title,
     std::auto_ptr<AttributeTableResult>
     (
       new AttributeTableResult
       (
	 list_of<string>
	  ("c0")
	  ("c1")
	  ("c2")
	  ("c3"),

	 list_of<AttributeTableResult::AttributeValue>
	  (m.c0)(m.c1)(m.c2)(m.c3),
	"Regression coefficients", "", ""
	)
     )
    )
    .setOrder(so.next());
       
  }

  arma::mat refdata_Ruu=refdatalib.getProfile("MKM_Channel", "180/Ruu_vs_yp");
  arma::mat refdata_Rvv=refdatalib.getProfile("MKM_Channel", "180/Rvv_vs_yp");
  arma::mat refdata_Rww=refdatalib.getProfile("MKM_Channel", "180/Rww_vs_yp");
  arma::mat refdata_Ruu395=refdatalib.getProfile("MKM_Channel", "395/Ruu_vs_yp");
  arma::mat refdata_Rvv395=refdatalib.getProfile("MKM_Channel", "395/Rvv_vs_yp");
  arma::mat refdata_Rww395=refdatalib.getProfile("MKM_Channel", "395/Rww_vs_yp");
  arma::mat refdata_Ruu590=refdatalib.getProfile("MKM_Channel", "590/Ruu_vs_yp");
  arma::mat refdata_Rvv590=refdatalib.getProfile("MKM_Channel", "590/Rvv_vs_yp");
  arma::mat refdata_Rww590=refdatalib.getProfile("MKM_Channel", "590/Rww_vs_yp");
  
  arma::mat refdata_K=refdata_Ruu;
  refdata_K.col(1)+=Interpolator(refdata_Rvv)(refdata_Ruu.col(0));
  refdata_K.col(1)+=Interpolator(refdata_Rww)(refdata_Ruu.col(0));
  refdata_K.col(1)*=0.5;
  refdata_K.col(0)/=180.0;
  
  arma::mat refdata_K395=refdata_Ruu395;
  refdata_K395.col(1)+=Interpolator(refdata_Rvv395)(refdata_Ruu395.col(0));
  refdata_K395.col(1)+=Interpolator(refdata_Rww395)(refdata_Ruu395.col(0));
  refdata_K395.col(1)*=0.5;
  refdata_K395.col(0)/=395.0;

  arma::mat refdata_K590=refdata_Ruu590;
  refdata_K590.col(1)+=Interpolator(refdata_Rvv590)(refdata_Ruu590.col(0));
  refdata_K590.col(1)+=Interpolator(refdata_Rww590)(refdata_Ruu590.col(0));
  refdata_K590.col(1)*=0.5;
  refdata_K590.col(0)/=590.0;
  

  // Mean reynolds stress profiles
  {
    string chart_name="chartMeanReyStress_"+title;

        arma::mat
            Rxx=arma::zeros(data.n_rows),
            Rxy=arma::zeros(data.n_rows),
            Rxz=arma::zeros(data.n_rows),
            Ryy=arma::zeros(data.n_rows),
            Ryz=arma::zeros(data.n_rows),
            Rzz=arma::zeros(data.n_rows);

        int c;
        if ( (c = cd.colIndex("UPrime2Mean")) >=0 )
        {
            Rxx += data.col(c);
            Rxy += data.col(c+1);
            Rxz += data.col(c+2);
            Ryy += data.col(c+3);
            Ryz += data.col(c+4);
            Rzz += data.col(c+5);
        }
        if ( (c = cd.colIndex("R")) >=0 )
        {
            Rxx += data.col(c);
            Rxy += data.col(c+1);
            Rxz += data.col(c+2);
            Ryy += data.col(c+3);
            Ryz += data.col(c+4);
            Rzz += data.col(c+5);
        }
    
    
    arma::mat yplus=Re_tau-Re_tau*data.col(0);
    
    arma::mat axial(		join_rows(yplus, Rxx)	);
    arma::mat wallnormal(	join_rows(yplus, Ryy)	);
    arma::mat spanwise(		join_rows(yplus, Rzz)	);
    arma::mat cross(		join_rows(yplus, Rxy)	);

    axial.save( 	( executionPath()/( "Raxial_vs_yp_"		+title+".txt") ).c_str(), arma::raw_ascii);
    wallnormal.save( 	( executionPath()/( "Rwallnormal_vs_yp_"	+title+".txt") ).c_str(), arma::raw_ascii);
    spanwise.save( 	( executionPath()/( "Rspanwise_vs_yp_"		+title+".txt") ).c_str(), arma::raw_ascii);
    
    
    std::vector<PlotCurve> plotcurves =
      list_of
       (PlotCurve(axial, 		"Ruu", "w l lt 1 lc -1 lw 2 t '$R_{uu}^+$'"))
       (PlotCurve(wallnormal, 	"Rvv", "w l lt 1 lc 1 lw 2 t '$R_{vv}^+$'"))
       (PlotCurve(spanwise, 	"Rww", "w l lt 1 lc 3 lw 2 t '$R_{ww}^+$'"))
       (PlotCurve(cross, 		"Ruv", "w l lt 1 lc 4 lw 2 t '$R_{uv}^+$'"))
       ;
       
    if (includeAllComponentsInCharts)
    {
      std::vector<PlotCurve> pc =
      list_of
        (PlotCurve(join_rows(yplus, Rxz), 		"Ruw", "w l lt 1 lc 5 t '$R_{uw}^+$'"))
        (PlotCurve(join_rows(yplus, Ryz), 		"Rvw", "w l lt 1 lc 5 t '$R_{vw}^+$'"))
        ;
      plotcurves.insert(plotcurves.end(), pc.begin(), pc.end());
    }
       
    if (includeRefDataInCharts)
    {
      std::vector<PlotCurve> pc =
      list_of
        (PlotCurve(refdata_Ruu, 		"RuuMKM180", "w l lt 2 lc -1 t '$R_{uu,ref}^+(Re_{\\tau}=180)$'"))
        (PlotCurve(refdata_Rvv, 		"RvvMKM180", "w l lt 2 lc 1 t '$R_{vv,ref}^+(Re_{\\tau}=180)$'"))
        (PlotCurve(refdata_Rww, 		"RwwMKM180", "w l lt 2 lc 3 t '$R_{ww,ref}^+(Re_{\\tau}=180)$'"))
        
        (PlotCurve(refdata_Ruu395, 	"RuuMKM395", "w l lt 4 lc -1 t '$R_{uu,ref}^+(Re_{\\tau}=395)$'"))
        (PlotCurve(refdata_Rvv395, 	"RvvMKM395", "w l lt 4 lc 1 t '$R_{vv,ref}^+(Re_{\\tau}=395)$'"))
        (PlotCurve(refdata_Rww395, 	"RwwMKM395", "w l lt 4 lc 3 t '$R_{ww,ref}^+(Re_{\\tau}=395)$'"))
        
        (PlotCurve(refdata_Ruu590, 	"RuuMKM590", "w l lt 3 lc -1 t '$R_{uu,ref}^+(Re_{\\tau}=590)$'"))
        (PlotCurve(refdata_Rvv590, 	"RvvMKM590", "w l lt 3 lc 1 t '$R_{vv,ref}^+(Re_{\\tau}=590)$'"))
        (PlotCurve(refdata_Rww590, 	"RwwMKM590", "w l lt 3 lc 3 t '$R_{ww,ref}^+(Re_{\\tau}=590)$'"))
        ;
      plotcurves.insert(plotcurves.end(), pc.begin(), pc.end());
    }
    
    std::string maxRp;
    if (includeRefDataInCharts)
    {
        maxRp="set yrange [:"+lexical_cast<string>(std::max( max(axial.col(1)), max(refdata_Ruu590.col(1)) ))+"]";
    }
    else
    {
        maxRp="";
    }
    
    addPlot
    (
      section, executionPath(), chart_name,
      "$y^+$", "$\\langle R^+ \\rangle$",
      plotcurves,
      "Wall normal profiles of averaged reynolds stresses at x/H=" + str(format("%g")%xByH),
      maxRp
    )
    .setOrder(so.next());

    chart_name="chartMeanTKE_"+title;
    
    int ck=cd["k"].col;
    
    arma::mat K= 0.5*( Rxx + Ryy + Rzz );
    if (cd.find("k")!=cd.end())
    {
      K+=data.col(ck);
    }
    else
      cout<<"not adding k"<<endl;
	  
    K /= pow(utau_, 2); // K => K+
    
    arma::mat Kp(join_rows( (1.-data.col(0)/(0.5*H)), K));
    Kp.save( (executionPath()/("Kp_vs_ydelta_"+title+".txt")).c_str(), arma::raw_ascii);
    
    addPlot
    (
      section, executionPath(), chart_name,
      "$y_{\\delta}$", "$\\langle k^+ \\rangle$",
      list_of
       (PlotCurve( Kp, 			"TKE", "w l lt -1 t 'TKE'" ))
       (PlotCurve( refdata_K, 		"TKEMKM180", "u 1:2 w l lt 1 lc 1 t 'DNS ($Re_{\\tau}=180$, MKM)'" ))
       (PlotCurve( refdata_K395, 	"TKEMKM395", "u 1:2 w l lt 2 lc 1 t 'DNS ($Re_{\\tau}=395$, MKM)'" ))
       (PlotCurve( refdata_K590, 	"TKEMKM590", "u 1:2 w l lt 3 lc 1 t 'DNS ($Re_{\\tau}=590$, MKM)'" ))
       ,
     "Wall normal profiles of averaged turbulent kinetic energy ($1/2 R_{ii} + k_{model}$) at x/H=" + str(format("%g")%xByH)
    )
    .setOrder(so.next());
  }

  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  if (!p.mesh.twod)
  {
    std::string extractSliceCmd;
    if (isfirstslice)
      extractSliceCmd="eb = extractPatches(cbi, '"+cycl_in_+"')\n";
    else      
      extractSliceCmd="eb = planarSlice(cbi, ["+lexical_cast<string>(x)+",0,1e-6], [1,0,0])\n";
    
    try 
    {
      std::string pressure_contour_name="contourPressure_ax_"+title;
      std::string pressure_contour_filename=pressure_contour_name+".png";
      runPvPython
      (
	cm, executionPath(), list_of<std::string>
	(
	  init+
	  extractSliceCmd+
	  "Show(eb)\n"
	  "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
	  "setCam([-10,0,0], [0,0,0], [0,1,0])\n"
	  "WriteImage('"+pressure_contour_filename+"')\n"
	)
      );
      section->insert(pressure_contour_name,
	std::auto_ptr<Image>(new Image
	(
	executionPath(), pressure_contour_filename, 
	"Contour of pressure (axial section at x/H=" + str(format("%g")%xByH)+")", ""
      ))) .setOrder(so.next());
    }
    catch (insight::Exception& e)
    {
      std::cout
	<<"Warning: creation of pressure contour plot failed! Error was: "
	<<e
	<<std::endl;
    }
    
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
	  extractSliceCmd+
	  "Show(eb)\n"
	  "displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	  "setCam([-10,0,0], [0,0,0], [0,1,0])\n"
	  "WriteImage('"+velocity_contour_filename+"')\n"
	)
      );
      section->insert(velocity_contour_name,
	std::auto_ptr<Image>(new Image
	(
	executionPath(), velocity_contour_filename, 
	"Contour of "+c+"-Velocity (axial section at x/H=" + str(format("%g")%xByH)+")", ""
      ))) .setOrder(so.next());
    }
  }
  
  results->insert(title, section) .setOrder(o.next());
}

ResultSetPtr ChannelBase::evaluateResults(OpenFOAMCase& cm)
{
  const ParameterSet& p=parameters_;
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  RFieldName_="UPrime2Mean";
  UMeanName_="UMean";
  if ( const RASModel *rm = cm.get<RASModel>(".*") )
  {
    std::cout<<"Case included RASModel "<<rm->name()<<". Computing R field"<<std::endl;
    cm.executeCommand( executionPath(), "R"/*, list_of("-latestTime")*/ );
    RFieldName_="R";
    UMeanName_="U";
  }
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm);
  Ordering o;
  
  const LinearTPCArray* tpcs=cm.get<LinearTPCArray>("tpc_interiorTPCArray");
  if (!tpcs)
  {
    //throw insight::Exception("tpc FO array not found in case!");
  }
  else
  {
    tpcs->evaluate
    (
      cm, executionPath(), results, 
      "two-point correlation of velocity at different radii at x/H="+str(format("%g")%(0.5*L/H))
    );
  }
 
  try
  {
    
  // Wall friction coefficient
  arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), nax_);
  wallforce.col(0)-=wallforce.col(0)(0); // ensure start curve starts at x=0
    
  arma::mat Cf_vs_xp(join_rows(
      wallforce.col(0)*Re_tau, 
      wallforce.col(1)/(0.5*pow(Ubulk_,2))
    ));
    Cf_vs_xp.save( (executionPath()/"Cf_vs_xp.txt").c_str(), arma::raw_ascii);
    
    arma::mat Cftheo_vs_xp=zeros(2,2);
    Cftheo_vs_xp(0,0)=Cf_vs_xp(0,0);
    Cftheo_vs_xp(1,0)=Cf_vs_xp(Cf_vs_xp.n_rows-1,0);
    double Cftheo=pow(utau_,2)/(0.5*pow(Ubulk_,2));
    Cftheo_vs_xp(0,1)=Cftheo;
    Cftheo_vs_xp(1,1)=Cftheo;

    addPlot
    (
      results, executionPath(), "chartMeanWallFriction",
      "$x^+$", "$\\langle C_f \\rangle$",
      list_of
	(PlotCurve(Cf_vs_xp, "cfd", "w l lt 1 lc -1 lw 2 t 'CFD'"))
	(PlotCurve(Cftheo_vs_xp, "ref", "w l lt 2 lc -1 lw 1 t 'Analytical'"))
	,
      "Axial profile of wall friction coefficient",
      "set key bottom right"
    ) .setOrder(o.next());    
  }
  catch (...)
  {
    insight::Warning("Could not include viscous resistance coefficient plot into result report.\nCheck console output for reason.");
  }
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
    
//   if (INSIGHT_ANALYSIS_STEP_NOT_DONE(*this, "EVAL_SNAPSHOT_P"))
//   {	
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
//   }
  results->insert("contourPressure",
    std::auto_ptr<Image>(new Image
    (
    executionPath(), "pressure_longi.jpg", 
    "Contour of pressure (longitudinal section)", ""
  ))) .setOrder(o.next());
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
//     if (INSIGHT_ANALYSIS_STEP_NOT_DONE(*this, "EVAL_SNAPSHOT_U"+c))
//     {
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
//     }
    results->insert("contourU"+c,
      std::auto_ptr<Image>(new Image
      (
      executionPath(), "U"+c+"_longi.jpg", 
      "Contour of "+c+"-Velocity", ""
    ))) .setOrder(o.next());
  }
  
  Ordering xseco(10.);
  evaluateAtSection(cm, results, 1e-4, 0, xseco);

  return results;
}




defineType(ChannelCyclic);

ChannelCyclic::ChannelCyclic(const ParameterSet& ps, const boost::filesystem::path& exepath)
: ChannelBase(ps, exepath)
{
}

ParameterSet ChannelCyclic::defaultParameters()
{
  ParameterSet p(ChannelBase::defaultParameters());
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
          
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
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}


void ChannelCyclic::createMesh
(
  OpenFOAMCase& cm
)
{  
  ChannelBase::createMesh(cm);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelCyclic::createCase
(
  OpenFOAMCase& cm
)
{  
  const ParameterSet& p=parameters_;
  // create local variables from ParameterSet
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
    
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  ChannelBase::createCase(cm);
  
  {
    std::vector<arma::mat> pl;
    double l=0.25*H;
    int np=25;
    for (int j=0; j<np; j++)
    {
      pl.push_back(vec3(l*double(j)/double(np-1), 0, 0));
    }
    cm.insert(new probes(cm, probes::Parameters()
    .set_fields( list_of<std::string>("p")("U") )
    .set_probeLocations(pl)
    .set_name("center_probes")
    ));
  }
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(Ubulk_, 0, 0))
		));

}

void ChannelCyclic::applyCustomPreprocessing(OpenFOAMCase& cm)
{
  const ParameterSet& p=parameters_;

  if (p.getBool("run/perturbU"))
  {
    PSDBL(p, "operation", Re_tau);

    cm.executeCommand(executionPath(), "perturbU", 
		      list_of<string>
		      (lexical_cast<string>(Re_tau))
		      ("("+lexical_cast<string>(Ubulk_)+" 0 0)") 
		    );
  }
  OpenFOAMAnalysis::applyCustomPreprocessing(cm);
}

void ChannelCyclic::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  const ParameterSet& p=parameters_;

  ChannelBase::applyCustomOptions(cm, dicts);
  
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
  controlDict["endTime"] = end_;
}

addToFactoryTable(Analysis, ChannelCyclic);
addToStaticFunctionTable(Analysis, ChannelCyclic, defaultParameters);




}
