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
#include "base/boost_include.h"

#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "refdata.h"

#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/boundaryconditions/simplebc.h"
#include "openfoam/caseelements/boundaryconditions/cyclicpairbc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/basic/pressuregradientsource.h"
#include "openfoam/caseelements/basic/rasmodel.h"

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

ChannelBase::supplementedInputData::supplementedInputData(
    std::unique_ptr<Parameters> pPtr,
    const boost::filesystem::path &/*workDir*/,
    ProgressDisplayer &progress)
  : supplementedInputDataDerived<Parameters>( std::move(pPtr) ),
    cycl_in_("cycl_half0"),
    cycl_out_("cycl_half1"),
    wall_up_("wall_upper"),
    wall_lo_("wall_lower")
{
  // Physics
  Re_ = Re(p().operation.Re_tau);
  Ubulk_ = Re_ / p().operation.Re_tau;
  T_ = p().geometry.L / Ubulk_;
  nu_ = 1. / p().operation.Re_tau;
  utau_ = p().operation.Re_tau*nu_ / (0.5*p().geometry.H);
  ywall_ = p().mesh.ypluswall / p().operation.Re_tau;

  // grid
  //double Delta=L/double(nax);
  nax_=std::max(1, int(p().geometry.L * p().operation.Re_tau / p().mesh.dxplus));

  if (p().mesh.twod)
    nb_=1;
  else
    nb_=std::max(1, int( p().geometry.B * p().operation.Re_tau/p().mesh.dzplus));

  hbuf_=0.0;
  nh_=std::max(1, p().mesh.nh/2);



  nhbuf_=0;
  gradl_=1.;
  if (p().mesh.fixbuf>0)
  {
    if (nh_-nhbuf_<=1)
      throw insight::Exception("Cannot fix cell height inside buffer layer: too few cells in vertical direction allowed! (min "+lexical_cast<string>(nhbuf_+1)+")");

    nhbuf_=p().mesh.nl;

    gradl_=pow(p().mesh.layerratio, p().mesh.nl-1);

    hbuf_=bmd::GradingAnalyzer(gradl_).calc_L(ywall_, p().mesh.nl);
    hbuf_=std::min(0.49*p().geometry.H, hbuf_);

//     double ypbuf=30.;
//     hbuf_=ypbuf/Re_tau;
//     nhbuf_=std::max(1.0, hbuf_/ywall_);
      //ywall_=hbuf_/double(nhbuf);

  }

  gradh_=bmd::GradingAnalyzer
  (
    bmd::GradingAnalyzer(gradl_).calc_delta1(ywall_),
    0.5*p().geometry.H-hbuf_,
    nh_-nhbuf_
  ).grad();
  //nh_=max(1, bmd::GradingAnalyzer(gradh_).calc_n(ywall_, H/2.));

  if (const auto *steady
        = boost::get<Parameters::run_type::regime_steady_type>(&p().run.regime))
  {
    end_=steady->iter;
    avgStart_=0.98*end_;
    avg2Start_=end_;
  }
  else if (const auto *unsteady
        = boost::get<Parameters::run_type::regime_unsteady_type>(&p().run.regime))
  {
    avgStart_=unsteady->inittime*T_;
    avg2Start_=avgStart_+unsteady->meantime*T_;
    end_=avg2Start_+unsteady->mean2time*T_;
  }

  int np=25;
  probe_locations_.clear();
  for (int j=0; j<np; j++)
  {
      probe_locations_.push_back(vec3(
        0.,
        -0.5*p().geometry.H + 0.5*p().geometry.H*(1.-::cos(0.5*M_PI*double(j)/double(np-1))),
        0.
      ));
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


ChannelBase::ChannelBase(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& progress)
: OpenFOAMAnalysis
  (
    "Channel Flow Test Case",
    "Rectangular domain with cyclic BCs on axial ends",
    ps, exepath
  ),
  parameters_( new supplementedInputData(
                 std::make_unique<Parameters>(ps),
                 exepath, progress
                 ) )
{}

ChannelBase::~ChannelBase()
{}


std::string ChannelBase::cyclPrefix() const
{
  boost::smatch m;
  boost::regex_search(sp().cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  return namePrefix;
}




void ChannelBase::calcDerivedInputData(ProgressDisplayer& progress)
{
  OpenFOAMAnalysis::calcDerivedInputData(progress);
  reportIntermediateParameter("gradl", sp().gradl_, "near-wall layer block grading");
  reportIntermediateParameter("hbuf", sp().hbuf_, "near-wall layer block height (clipped to 0.9*H)");
}




void ChannelBase::createMesh
(
  OpenFOAMCase& cm, ProgressDisplayer& progress
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();

  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
//   bmd->setDefaultPatch("walls", "wall");
  
  
//  double al = M_PI/2.;
  
  std::map<int, Point> pts = {
      {0, 	vec3(0.5*p().geometry.L, -0.5*p().geometry.H, -0.5*p().geometry.B)},
      {1, 	vec3(-0.5*p().geometry.L, -0.5*p().geometry.H, -0.5*p().geometry.B)},
      {2, 	vec3(-0.5*p().geometry.L, -0.5*p().geometry.H, 0.5*p().geometry.B)},
      {3, 	vec3(0.5*p().geometry.L, -0.5*p().geometry.H, 0.5*p().geometry.B)}
  };
  arma::mat vHbuf=vec3(0, 0, 0);
  arma::mat vH=vec3(0, p().geometry.H, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(sp().cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(sp().cycl_out_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  string side_type="cyclic";
  if (p().mesh.twod) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch("cycl_side", new Patch(side_type));
  
  Patch& wall_up= 	bmd->addPatch(sp().wall_up_, new Patch("wall"));
  wall_up.addFace(pts[0]+vH, pts[1]+vH, pts[2]+vH, pts[3]+vH);
  Patch& wall_lo= 	bmd->addPatch(sp().wall_lo_, new Patch("wall"));
  wall_lo.addFace(pts[0], pts[1], pts[2], pts[3]);
  
  int nh=sp().nh_;
  
  if (sp().nhbuf_>0)
  {
    vHbuf=vec3(0, sp().hbuf_, 0);
    vH=vec3(0, p().geometry.H - 2.*sp().hbuf_, 0);
    nh=sp().nh_-sp().nhbuf_;
    
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  pts[0], pts[1], pts[2], pts[3],
	  (pts[0])+vHbuf, (pts[1])+vHbuf, (pts[2])+vHbuf, (pts[3])+vHbuf
	  ),
          sp().nax_, sp().nb_, sp().nhbuf_,
          { 1., 1., sp().gradl_ }
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
          sp().nax_, sp().nb_, sp().nhbuf_,
          { 1., 1., 1./sp().gradl_ }
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
        sp().nax_, sp().nb_, nh,
        { 1., 1., sp().gradh_ }
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
        sp().nax_, sp().nb_, nh,
        { 1., 1., 1./sp().gradh_ }
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
  OpenFOAMCase& cm, ProgressDisplayer& progress
)
{
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

//   cm.insert( new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
//   ) );
  
  if (const auto *steady
        = boost::get<Parameters::run_type::regime_steady_type>(&p().run.regime))
  {
    cm.insert(new steadyIncompressibleNumerics(cm, steadyIncompressibleNumerics::Parameters()
      .set_checkResiduals(false) // don't stop earlier since averaging should be completed
      .set_Uinternal(vec3(sp().Ubulk_,0,0))
      .set_endTime(sp().end_)
      .set_decompWeights(vec3(2,1,0))
      .set_np(p().OpenFOAMAnalysis::Parameters::run.np)
      .set_decompositionMethod(decomposeParDict::Parameters::decompositionMethod_type::hierarchical)
    ));
  } 
  else if (const auto *unsteady
        = boost::get<Parameters::run_type::regime_unsteady_type>(&p().run.regime))
  {
    cm.insert( new unsteadyIncompressibleNumerics(cm, unsteadyIncompressibleNumerics::Parameters()
      .set_LESfilteredConvection(p().run.filteredconvection)
//      .set_maxDeltaT(0.25*T_)
      .set_time_integration(PIMPLESettings::Parameters()
        .set_timestep_control(PIMPLESettings::Parameters::timestep_control_adjust_type(
          1,
          0.25*sp().T_
        ))
       )
      .set_Uinternal(vec3(sp().Ubulk_,0,0))
      .set_writeControl(FVNumerics::Parameters::writeControl_type::adjustableRunTime)
      .set_writeInterval(0.25*sp().T_)
      .set_endTime( sp().end_ )
      .set_writeFormat(FVNumerics::Parameters::writeFormat_type::ascii)
      .set_deltaT( double(p().geometry.L/sp().nax_)/sp().Ubulk_ ) // Co=1
      .set_decompWeights(vec3(2,1,0))
      .set_decompositionMethod(decomposeParDict::Parameters::decompositionMethod_type::simple)
      .set_np(p().OpenFOAMAnalysis::Parameters::run.np)
    ));
  }
  
  std::vector<std::string> fields_to_average = { "p", "U", "pressureForce", "viscousForce" };
  
  if (p().operation.wscalar)
  {
    cm.insert(new PassiveScalar(cm, PassiveScalar::Parameters()
        .set_fieldname("theta")
    ));
    fields_to_average.push_back("theta");
    multiphaseBC::multiphaseBCPtr temp_hi(new multiphaseBC::uniformWallTiedPhases( multiphaseBC::uniformWallTiedPhases::mixture(
        map_list_of("theta", 1.0)
    )));
    multiphaseBC::multiphaseBCPtr temp_lo(new multiphaseBC::uniformWallTiedPhases( multiphaseBC::uniformWallTiedPhases::mixture(
        map_list_of("theta", 0.0)
    )));
    
    cm.insert(new WallBC(cm, sp().wall_lo_, boundaryDict, WallBC::Parameters()
     .set_phasefractions(temp_hi) 
    ));
    cm.insert(new WallBC(cm, sp().wall_up_, boundaryDict, WallBC::Parameters()
     .set_phasefractions(temp_lo) 
    ));
  }
  
  cm.insert(new extendedForces(cm, extendedForces::Parameters()
    .set_patches( list_of<string>("\"wall_.*\"") )
  ));

  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_fields(fields_to_average)
    .set_timeStart(sp().avgStart_)
    .set_name("zzzaveraging") // shall be last FO in list
  ));
  
  if (p().run.eval2)
  {
    cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
      .set_R(0.5*p().geometry.H)
//       .set_x(0.0) // middle x==0!
//       .set_z(-0.49*B)
      .set_p0(vec3(0., 0., -0.49*p().geometry.B))
      .set_axSpan(0.5*p().geometry.L)
      .set_tanSpan(0.45*p().geometry.B)
      .set_name("tpc_interior")
      .set_timeStart( sp().avg2Start_ )
    ));
  }
  
  {
    std::vector<std::string> sample_fields = { "p", "U" };
    
    if (p().operation.wscalar)
    {
        sample_fields.push_back("theta");
    }
    
    cm.insert(new probes(cm, probes::Parameters()
    .set_fields( sample_fields )
    .set_probeLocations(sp().probe_locations_)
    .set_name("center_probes")
    .set_outputControl("timeStep")
    .set_outputInterval(10.0)
    ));
  }
  
  cm.insert(new singlePhaseTransportProperties(
              cm,
              singlePhaseTransportProperties::Parameters().set_nu(sp().nu_) ));

  if (p().mesh.twod)
    cm.insert(new SimpleBC(cm, "cycl_side", boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, "cycl_side", boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters()
                             .set_roughness_z0(p().operation.y0)
                             );
  
  insertTurbulenceModel(cm, p().fluid.turbulenceModel);
}




void ChannelBase::applyCustomOptions(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
  OpenFOAMAnalysis::applyCustomOptions(cm, dicts);
  
  if (p().mesh.twod)
  {
    OFDictData::dict& fvSolution=dicts->lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers["p"]=cm.stdSymmSolverSetup(1e-7, 0.01);
  }
//   OFDictData::dictFile& controlDict=dicts->lookupDict("system/controlDict");
//   controlDict["maxDeltaT"]=0.5*T_;
}




void ChannelBase::evaluateAtSection(
  OpenFOAMCase& cm, 
  ResultSetPtr results, double x, int /*i*/,
  Ordering& o,
  bool includeRefDataInCharts,
  bool includeAllComponentsInCharts,
  const std::string& vertical_probes_array_name
)
{
  double xByH= (x/p().geometry.L + 0.5)*p().geometry.L/p().geometry.H;
  bool isfirstslice=false;
  if (xByH<=1e-3) isfirstslice=true;

  std::shared_ptr<ResultSection> section
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
  
  if (vertical_probes_array_name!="")
  {
      try
      {
        arma::cube U_vs_t = probes::readProbes(cm, executionPath(), vertical_probes_array_name, "U");
        
        arma::mat yp=arma::zeros(sp().probe_locations_.size(),1);
        for(size_t i=0; i<sp().probe_locations_.size(); i++) yp(i)=p().operation.Re_tau*(1.+sp().probe_locations_[i](1));
        arma::uword npts=yp.n_elem;
        arma::uword ictr=npts-1;
        
        arma::mat t, U[3], U_mean[3], U_var[3], Uprime[3];
        for(arma::uword i=0; i<3; i++)
        {
            arma::mat t_full=U_vs_t.slice(i).col(0);
            arma::uvec valid_rows=arma::find(t_full>sp().avgStart_);
            t=t_full.rows(valid_rows);
            arma::mat U_full=U_vs_t.slice(i).cols(1, npts);
            U[i]=U_full.rows(valid_rows);
            U_mean[i]= arma::mean(U[i]);
            Uprime[i] = U[i] - (arma::ones(U[i].n_rows, 1) * U_mean[i]);
            U_var[i]= arma::mean(Uprime[i] % Uprime[i]).t();
        }

        // output time history of u in centerline
        addPlot
        (
            section, executionPath(), "chartUVarianceCenter",
            "$t$", "$U$",
            {
              PlotCurve( t, U[0].col(ictr),                          "Ux_vs_t", "w l lt -1 lc 1 t '$U_x$'" ),
              PlotCurve( t, U_mean[0](ictr),                         "Uxmean",  "w l lt -1 lc 1 lw 2 t '$\\langle U_x \\rangle$'" ),
              PlotCurve( t, U_mean[0](ictr) + sqrt(U_var[0](ictr)),  "Uxvar_up",  "w l lt -1 lc 1 dt 2 t '$\\langle u^{\\prime 2}_x \\rangle$'" ),
              PlotCurve( t, U_mean[0](ictr) - sqrt(U_var[0](ictr)), 	"Uxvar_lo",  "w l lt -1 lc 1 dt 2 not" ),

              PlotCurve( t, U[1].col(ictr),			                "Uy_vs_t", "w l lt -1 lc 2 t '$U_y$'" ),
              PlotCurve( t, U_mean[1](ictr),                         "Uymean",  "w l lt -1 lc 2 lw 2 t '$\\langle U_y \\rangle$'" ),
              PlotCurve( t, U_mean[1](ictr) + sqrt(U_var[1](ictr)), 	"Uyvar_up",  "w l lt -1 lc 2 dt 2 t '$\\langle u^{\\prime 2}_y \\rangle$'" ),
              PlotCurve( t, U_mean[1](ictr) - sqrt(U_var[1](ictr)), 	"Uyvar_lo",  "w l lt -1 lc 2 dt 2 not" ),

              PlotCurve( t, U[2].col(ictr),			                "Uz_vs_t", "w l lt -1 lc 4 t '$U_z$'" ),
              PlotCurve( t, U_mean[2](ictr),                         "Uzmean",  "w l lt -1 lc 4 lw 2 t '$\\langle U_z \\rangle$'" ),
              PlotCurve( t, U_mean[2](ictr) + sqrt(U_var[2](ictr)), 	"Uzvar_up",  "w l lt -1 lc 4 dt 2 t '$\\langle u^{\\prime 2}_z \\rangle$'" ),
              PlotCurve( t, U_mean[2](ictr) - sqrt(U_var[2](ictr)), 	"Uzvar_lo",  "w l lt -1 lc 4 dt 2 not" )
            },
            ""
        )
        .setOrder(so.next());

        // output time history of u in centerline
        addPlot
        (
            section, executionPath(), "chartUVariance",
            "$y^+$", "$\\langle u^{\\prime 2} \\rangle$",
            {
              PlotCurve( yp, U_var[0],  "Uxvar_vs_yp", "w l lt -1 lc 1 t '$\\langle u^{\\prime 2} \\rangle$'" ),
              PlotCurve( yp, U_var[1],  "Uyvar_vs_yp", "w l lt -1 lc 2 t '$\\langle u^{\\prime 2} \\rangle$'" ),
              PlotCurve( yp, U_var[2],  "Uzvar_vs_yp", "w l lt -1 lc 4 t '$\\langle u^{\\prime 2} \\rangle$'" )
            },
        ""
        );
            
        if (p().operation.wscalar)
        {
            arma::mat s_vs_t = probes::readProbes(cm, executionPath(), vertical_probes_array_name, "theta").slice(0);
            arma::mat t_full=s_vs_t.col(0);
            arma::uvec valid_rows=arma::find(t_full>sp().avgStart_);
            arma::mat t=t_full.rows(valid_rows);
            arma::mat s_full=s_vs_t.cols(1,npts);
            arma::mat s=s_full.rows(valid_rows);
            arma::mat s_mean = arma::mean(s);
            arma::mat sprime = s - (arma::ones(s.n_rows, 1) * s_mean);
            
            arma::mat s_var = arma::mean(sprime % sprime).t();
            
            arma::mat s_flux[3];
            for (int i=0; i<3; i++)
            {
                s_flux[i] = arma::mean(Uprime[i] % sprime).t();
            }
            
            addPlot
            (
                section, executionPath(), "chartThetaVariance",
                "$y^+$", "$\\langle u^{\\prime 2} \\rangle$",
                {
                  PlotCurve( yp, s_var,            "svar_vs_yp",   "w l lt -1 lc 1 t '$\\langle \\vartheta^{\\prime 2}/\\vartheta_{max} \\rangle$'" ),
                  PlotCurve( yp, s_flux[0]/sp().utau_,  "sfluxx_vs_yp", "w l lt -1 lc 2 t '$\\langle u^{\\prime}_x \\vartheta^{\\prime} \\rangle/(u_\\tau \\vartheta_{max})$'" ),
                  PlotCurve( yp, s_flux[1]/sp().utau_,  "sfluxy_vs_yp", "w l lt -1 lc 4 t '$\\langle u^{\\prime}_y \\vartheta^{\\prime} \\rangle/(u_\\tau \\vartheta_{max})$'" ),
                  PlotCurve( yp, s_flux[2]/sp().utau_,  "sfluxz_vs_yp", "w l lt -1 lc 6 t '$\\langle u^{\\prime}_z \\vartheta^{\\prime} \\rangle/(u_\\tau \\vartheta_{max})$'" )
                },
                ""
            );
        }
      }
      catch (const std::exception& e)
      {
          insight::Warning("Could not evaluate probes \""+vertical_probes_array_name+"\", Reason: "+e.what()+"\n=> Skipping.\n");
      }
  }
  
    
  boost::ptr_vector<sampleOps::set> sets;
  
  double delta_yp1=1./p().operation.Re_tau;
  
  double
    miny=delta_yp1,
    maxy=0.5*p().geometry.H-delta_yp1;
    
  arma::mat pts = (miny + (maxy-miny)*cos(0.5*M_PI*(pow( linspace(0., 1., 101)-1.0, 2 )))) * vec3(0,1,0).t();
 
  pts.col(0)+=x;
  pts.col(2)+=-0.49*p().geometry.B;
 
  sets.push_back(new sampleOps::linearAveragedPolyLine(sampleOps::linearAveragedPolyLine::Parameters()
    .set_points( pts )
    .set_dir1(vec3(1,0,0))
    .set_dir2(vec3(0,0,0.98*p().geometry.B))
    .set_nd1(1)
    .set_nd2(sp().n_hom_avg)
    .set_name("radial")
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
    arma::uword c( cd[UMeanName_].col );
    
    arma::mat axial(join_rows(p().operation.Re_tau-p().operation.Re_tau*data.col(0), data.col(c)));
    arma::mat wallnormal(join_rows(p().operation.Re_tau-p().operation.Re_tau*data.col(0), data.col(c+1)));
    arma::mat spanwise(join_rows(p().operation.Re_tau-p().operation.Re_tau*data.col(0), data.col(c+2)));
    
    axial.save( (executionPath()/("umeanaxial_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    wallnormal.save( (executionPath()/("umeanwallnormal_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    spanwise.save( (executionPath()/("umeanspanwise_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    
    PlotCurveList plotcurves({
        PlotCurve(axial, 		"U", "w l lt 1 lc -1 lw 2 t '$U^+$'"),
        PlotCurve(wallnormal, 	"V", "w l lt 1 lc 1 lw 2 t '$V^+$'"),
        PlotCurve(spanwise, 		"W", "w l lt 1 lc 3 lw 2 t '$W^+$'")
      });
      
    if (includeRefDataInCharts)
    {
      PlotCurveList pc({
        PlotCurve(refdata_umean180,	"UMKM180", "w l lt 2 lc -1 t '$U_{ref}^+(Re_{\\tau}=180)$'"),
        PlotCurve(refdata_wmean180, 	"WMKM180", "w l lt 2 lc 3 t '$W_{ref}^+(Re_{\\tau}=180)$'"),
        PlotCurve(refdata_umean395, 	"UMKM395", "w l lt 4 lc -1 t '$U_{ref}^+(Re_{\\tau}=395)$'"),
        PlotCurve(refdata_wmean395, 	"WMKM395", "w l lt 4 lc 3 t '$W_{ref}^+(Re_{\\tau}=395)$'"),
        PlotCurve(refdata_umean590, 	"UMKM590", "w l lt 3 lc -1 t '$U_{ref}^+(Re_{\\tau}=590)$'"),
        PlotCurve(refdata_wmean590, 	"WMKM590", "w l lt 3 lc 3 t '$W_{ref}^+(Re_{\\tau}=590)$'")
      });
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
    
    arma::mat ydelta=1.0-(data.col(0)+delta_yp1)/(0.5*p().geometry.H); //Re_tau-Re_tau*data.col(0);
    arma::mat Lt1=(2./p().geometry.H)*sqrt(k)/(0.09*omega);
    arma::mat Lt2=ydelta*0.5*p().geometry.H*0.41;
    //arma::mat Lt=arma::min(Lt1, Lt2);
    arma::mat Lt=Lt1;
    for (arma::uword i=0; i<Lt2.n_rows; i++) Lt(i)=min(Lt(i), Lt2(i));
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
      {
       PlotCurve(arma::mat(join_rows(ydelta, Lt1)), "cfdkO", "w l lt 2 lc 1 lw 1 t 'CFD (from k and omega)'"),
       PlotCurve(arma::mat(join_rows(ydelta, Lt2)), "Lmix", "w l lt 3 lc 1 lw 1 t 'Mixing length limit'"),
       PlotCurve(Ltp, "cfd", "w l lt 1 lc 1 lw 2 t 'CFD'"),
       PlotCurve(arma::mat(join_rows(ydelta, yfit)), "fit", "w l lt 2 lc 2 lw 2 t 'Fit'")
      },
      "Wall normal profile of turbulent length scale at $x/H=" + str(format("%g")%xByH) + "$. Fit: $"
		    + 	    str(format("%.5g") % m.c0)+" y_{\\delta}^{"+str(format("%.5g") % m.c2)+"}"
		    +" + ("+str(format("%.5g") % m.c1)+" y_{\\delta}^{"+str(format("%.5g") % m.c3)+"})$",
      "set key top left"
    )
    .setOrder(so.next());

    section->insert
    (
     "regressionCoefficientsTubulentLengthScale_"+title,
     std::unique_ptr<AttributeTableResult>
     (
       new AttributeTableResult
       (
         {
          "c0",
          "c1",
          "c2",
          "c3"
         },
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
    
    
    arma::mat yplus=p().operation.Re_tau-p().operation.Re_tau*data.col(0);
    
    arma::mat axial(		join_rows(yplus, Rxx)	);
    arma::mat wallnormal(	join_rows(yplus, Ryy)	);
    arma::mat spanwise(		join_rows(yplus, Rzz)	);
    arma::mat cross(		join_rows(yplus, Rxy)	);

    axial.save( 	( executionPath()/( "Raxial_vs_yp_"		+title+".txt") ).c_str(), arma::raw_ascii);
    wallnormal.save( 	( executionPath()/( "Rwallnormal_vs_yp_"	+title+".txt") ).c_str(), arma::raw_ascii);
    spanwise.save( 	( executionPath()/( "Rspanwise_vs_yp_"		+title+".txt") ).c_str(), arma::raw_ascii);
    
    
    PlotCurveList plotcurves({
       PlotCurve(axial, 	"Ruu", "w l lt 1 lc -1 lw 2 t '$R_{uu}^+$'"),
       PlotCurve(wallnormal, 	"Rvv", "w l lt 1 lc 1 lw 2 t '$R_{vv}^+$'"),
       PlotCurve(spanwise, 	"Rww", "w l lt 1 lc 3 lw 2 t '$R_{ww}^+$'"),
       PlotCurve(cross, 	"Ruv", "w l lt 1 lc 4 lw 2 t '$R_{uv}^+$'")
      });
       
    if (includeAllComponentsInCharts)
    {
      PlotCurveList pc({
        PlotCurve(join_rows(yplus, Rxz), "Ruw", "w l lt 1 lc 5 t '$R_{uw}^+$'"),
        PlotCurve(join_rows(yplus, Ryz), "Rvw", "w l lt 1 lc 5 t '$R_{vw}^+$'")
      });
      plotcurves.insert(plotcurves.end(), pc.begin(), pc.end());
    }
       
    if (includeRefDataInCharts)
    {
      PlotCurveList pc({
        PlotCurve(refdata_Ruu, 	"RuuMKM180", "w l lt 2 dt 2 lc -1 t '$R_{uu,ref}^+(Re_{\\tau}=180)$'"),
        PlotCurve(refdata_Rvv, 	"RvvMKM180", "w l lt 2 dt 2 lc 1 t '$R_{vv,ref}^+(Re_{\\tau}=180)$'"),
        PlotCurve(refdata_Rww, 	"RwwMKM180", "w l lt 2 dt 2 lc 3 t '$R_{ww,ref}^+(Re_{\\tau}=180)$'"),
        
        PlotCurve(refdata_Ruu395, 	"RuuMKM395", "w l lt 4 dt 2 lc -1 t '$R_{uu,ref}^+(Re_{\\tau}=395)$'"),
        PlotCurve(refdata_Rvv395, 	"RvvMKM395", "w l lt 4 dt 2 lc 1 t '$R_{vv,ref}^+(Re_{\\tau}=395)$'"),
        PlotCurve(refdata_Rww395, 	"RwwMKM395", "w l lt 4 dt 2 lc 3 t '$R_{ww,ref}^+(Re_{\\tau}=395)$'"),
        
        PlotCurve(refdata_Ruu590, 	"RuuMKM590", "w l lt 3 dt 2 lc -1 t '$R_{uu,ref}^+(Re_{\\tau}=590)$'"),
        PlotCurve(refdata_Rvv590, 	"RvvMKM590", "w l lt 3 dt 2 lc 1 t '$R_{vv,ref}^+(Re_{\\tau}=590)$'"),
        PlotCurve(refdata_Rww590, 	"RwwMKM590", "w l lt 3 dt 2 lc 3 t '$R_{ww,ref}^+(Re_{\\tau}=590)$'")
       });
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
    
    arma::uword ck=cd["k"].col;
    
    PlotCurveList kplots({
     PlotCurve( refdata_K, 	"TKEMKM180", "u 1:2 w l lt 1 dt 2 lc 1 t 'DNS ($Re_{\\tau}=180$, MKM)'" ),
     PlotCurve( refdata_K395, 	"TKEMKM395", "u 1:2 w l lt 2 dt 2 lc 1 t 'DNS ($Re_{\\tau}=395$, MKM)'" ),
     PlotCurve( refdata_K590, 	"TKEMKM590", "u 1:2 w l lt 3 dt 2 lc 1 t 'DNS ($Re_{\\tau}=590$, MKM)'" )
    });

    arma::mat kres= 0.5*( Rxx + Ryy + Rzz ) / pow(sp().utau_, 2);
    {
      arma::mat Kp(join_rows( (1.-data.col(0)/(0.5*p().geometry.H)), kres));

      kplots.push_back(PlotCurve( Kp, 	"TKEresolved", "w l lt 1 dt 1 lc 2 t 'TKE (resolved)'" ));
      Kp.save( (executionPath()/("Kpresolved_vs_ydelta_"+title+".txt")).c_str(), arma::raw_ascii);
    }

    if (cd.find("k")!=cd.end())
    {
      arma::mat K=kres + data.col(ck)/pow(sp().utau_, 2);
      arma::mat Kp(join_rows( (1.-data.col(0)/(0.5*p().geometry.H)), K));

      kplots.push_back(PlotCurve( Kp, 	"TKE", "w l lt -1 lw 2 t 'TKE'" ));
      Kp.save( (executionPath()/("Kp_vs_ydelta_"+title+".txt")).c_str(), arma::raw_ascii);
    }
	  
    
    addPlot
    (
      section, executionPath(), chart_name,
      "$y_{\\delta}$", "$\\langle k^+ \\rangle$",
      kplots,
     "Wall normal profiles of averaged turbulent kinetic energy ($1/2 R_{ii} + k_{model}$) at x/H=" + str(format("%g")%xByH)
    )
    .setOrder(so.next());
  }

  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  if (!p().mesh.twod)
  {
    std::string extractSliceCmd;
    if (isfirstslice)
      extractSliceCmd="eb = extractPatches(cbi, '"+sp().cycl_in_+"')\n";
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
        std::unique_ptr<Image>(new Image
	(
	executionPath(), pressure_contour_filename, 
	"Contour of pressure (axial section at x/H=" + str(format("%g")%xByH)+")", ""
      ))) .setOrder(so.next());
    }
    catch (const std::exception& e)
    {
      insight::Warning(
        std::string("Warning: creation of pressure contour plot failed! Error was: ")
        +e.what());
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
        std::unique_ptr<Image>(new Image
	(
	executionPath(), velocity_contour_filename, 
	"Contour of "+c+"-Velocity (axial section at x/H=" + str(format("%g")%xByH)+")", ""
      ))) .setOrder(so.next());
    }
  }
  

  results->insert(title, section) .setOrder(o.next());
}




ResultSetPtr ChannelBase::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& progress)
{  
  RFieldName_="UPrime2Mean";
  UMeanName_="UMean";
  if ( const RASModel *rm = cm.get<RASModel>(".*") )
  {
    std::cout<<"Case included RASModel "<<rm->name()<<". Computing R field"<<std::endl;
    calcR(cm, executionPath());
    RFieldName_="R";
    UMeanName_="U";
  }
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, progress);
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
      "two-point correlation of velocity at different radii at x/H="
          +str( format("%g") % (0.5*p().geometry.L/p().geometry.H) )
    );
  }

  try
  {
    
  // Wall friction coefficient
  arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), sp().nax_);
  wallforce.col(0)-=wallforce.col(0)(0); // ensure start curve starts at x=0
    
  arma::mat Cf_vs_xp(join_rows(
      wallforce.col(0)*p().operation.Re_tau,
      wallforce.col(1)/(0.5*pow(sp().Ubulk_,2))
    ));
    Cf_vs_xp.save( (executionPath()/"Cf_vs_xp.txt").c_str(), arma::raw_ascii);
    
    arma::mat Cftheo_vs_xp=zeros(2,2);
    Cftheo_vs_xp(0,0)=Cf_vs_xp(0,0);
    Cftheo_vs_xp(1,0)=Cf_vs_xp(Cf_vs_xp.n_rows-1,0);
    double Cftheo=pow(sp().utau_,2)/(0.5*pow(sp().Ubulk_,2));
    Cftheo_vs_xp(0,1)=Cftheo;
    Cftheo_vs_xp(1,1)=Cftheo;

    addPlot
    (
      results, executionPath(), "chartMeanWallFriction",
      "$x^+$", "$\\langle C_f \\rangle$",
      {
        PlotCurve(Cf_vs_xp, "cfd", "w l lt 1 lc -1 lw 2 t 'CFD'"),
        PlotCurve(Cftheo_vs_xp, "ref", "w l lt 2 lc -1 lw 1 t 'Analytical'")
      },
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
    std::unique_ptr<Image>(new Image
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
      std::unique_ptr<Image>(new Image
      (
      executionPath(), "U"+c+"_longi.jpg", 
      "Contour of "+c+"-Velocity", ""
    ))) .setOrder(o.next());
  }

  Ordering xseco(10.);
  evaluateAtSection(cm, results, 1e-4, 0, xseco);

  return results;
}




ChannelCyclic::ChannelCyclic(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& progress)
: ChannelBase(ps, exepath, progress)
{
}


void ChannelCyclic::createMesh
(
  OpenFOAMCase& cm, ProgressDisplayer& progress
)
{  
  ChannelBase::createMesh(cm, progress);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelCyclic::createCase
(
  OpenFOAMCase& cm, ProgressDisplayer& progress
)
{
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  ChannelBase::createCase(cm, progress);
  
  cm.insert(new PressureGradientSource(
              cm,
              PressureGradientSource::Parameters()
              .set_Ubar(vec3(sp().Ubulk_, 0, 0))
              ));
}


void ChannelCyclic::applyCustomPreprocessing(OpenFOAMCase& cm, ProgressDisplayer& progress)
{
  if (p().run.perturbU)
  {
    if (!cm.outputTimesPresentOnDisk(executionPath()))
    {
        cm.executeCommand(executionPath(), "perturbU", 
                {
                 { lexical_cast<string>(p().operation.Re_tau) },
                 { "("+lexical_cast<string>(sp().Ubulk_)+" 0 0)" }
                } );
    }
  }
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, progress);
}

void ChannelCyclic::applyCustomOptions(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
{
//  const ParameterSet& p=parameters_;

  ChannelBase::applyCustomOptions(cm, dicts);
  
  OFDictData::dictFile& decomposeParDict=dicts->lookupDict("system/decomposeParDict");
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

  OFDictData::dictFile& controlDict=dicts->lookupDict("system/controlDict");
  if (cm.OFversion()<=160)
  {
    controlDict["application"]="channelFoam";
  }
  controlDict["endTime"] = sp().end_;
}

addToAnalysisFactoryTable(ChannelCyclic);



}
