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
#include "base/boost_include.h"

#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"

#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/simplebc.h"
#include "openfoam/caseelements/boundaryconditions/cyclicpairbc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/basic/rasmodel.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/analysiscaseelements.h"


#ifdef HAS_REFDATA
#include "refdata.h"
#endif

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight 
{
  
addToAnalysisFactoryTable(FlatPlateBL);

const std::vector<double> FlatPlateBL::supplementedInputData::sec_locs_
 = { 0.01, 0.05, 0.1, 0.2, 0.5, 0.7, 0.9 };
  

FlatPlateBL::supplementedInputData::supplementedInputData(
    std::unique_ptr<Parameters> pPtr,
    const boost::filesystem::path &/*workDir*/,
    ProgressDisplayer &progress)
  : supplementedInputDataDerived<Parameters>( std::move(pPtr) )
{
  Retheta0_= p().operation.Retheta0;
  Rex_0_=Rex(Retheta0_);

  uinf_= p().operation.uinf;

  // estimate (turbulent) friction coefficient at initial location
  cf_0_=cf(Rex_0_);
  utau_0_=uinf_*sqrt(0.5*cf_0_);

  theta0_=Retheta0_*p().fluid.nu/uinf_;
  delta99_0_=Redelta99(Rex_0_)*p().fluid.nu/uinf_;


  in_="inlet";
  out_top_="outlet_top";
  cycl_prefix_="cyclic";
  approach_="approach";
  trip_="trip";

  L_ = p().geometry.LbyDelta99 * delta99_0_;

  Rex_e_=Rex_0_ + uinf_*L_/p().fluid.nu;

  /**
   * compute estimated BL thicknesses
   */
  double Retau_0=utau_0_*delta99_0_/p().fluid.nu;


  double thetae=Redelta2(Rex_e_)*p().fluid.nu/uinf_;
  double cf_e=cf(Rex_e_);
  double tau_e=cf_e*0.5*pow(uinf_,2);
  double utau_e=sqrt(tau_e);


  ypfac_ref_=sqrt(cf_0_/2.)*uinf_/p().fluid.nu;

  H_=p().geometry.HbyDelta99*delta99_0_;

  W_=p().geometry.WbyDelta99*delta99_0_;

  deltaywall_ref_=p().mesh.yplus0/ypfac_ref_;

  gradl_=pow(p().mesh.layerratio, p().mesh.nl-1);

  y_final_=bmd::GradingAnalyzer(gradl_).calc_L(deltaywall_ref_, p().mesh.nl);
  y_final_=std::min(0.9*H_, y_final_);

  gradh_=bmd::GradingAnalyzer
  (
    bmd::GradingAnalyzer(gradl_).calc_delta1(deltaywall_ref_),
    H_-y_final_,
    p().mesh.nh-p().mesh.nl
  ).grad();

  double deltax=(p().mesh.dxplus0/ypfac_ref_);



  nax_=std::max(1, int(round(L_/deltax)));


  if (p().mesh.twod)
    nlat_=1;
  else
    nlat_=std::max(1, int(round(W_/(p().mesh.dzplus0/ypfac_ref_))));

  T_=L_/uinf_;

//   std::string regime = p.get<SelectableSubsetParameter>("run/regime").selection();
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

  n_hom_avg_=std::max(1, nlat_-2);
}


FlatPlateBL::FlatPlateBL(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& progress)
: OpenFOAMAnalysis
  (
    "Flat Plate Boundary Layer Test Case",
    "Flat Plate with Evolving Boundary Layer",
    ps, exepath
  ),
  parameters_( std::make_unique<supplementedInputData>(
                 std::make_unique<Parameters>(ps),
                 exepath, progress
                 ) )
{}


FlatPlateBL::FlatPlateBL(
        std::unique_ptr<supplementedInputData> pPtr,
        const boost::filesystem::path& exepath,
        const std::string& name,
        const std::string& description )
    : OpenFOAMAnalysis
      (
        name,
        description,
        pPtr->p(), exepath
      ),
      parameters_( std::move(pPtr) )
{}


void FlatPlateBL::calcDerivedInputData(ProgressDisplayer& progress)
{
  insight::OpenFOAMAnalysis::calcDerivedInputData(progress);

  reportIntermediateParameter("uinf",       sp().uinf_, "free stream velocity", "m/s");
  reportIntermediateParameter("Retheta0",   sp().Retheta0_, "Momentum thickness Reynolds number at the inlet", "");
  reportIntermediateParameter("Rex_0",      sp().Rex_0_, "Reynolds number with turbulent running length at the inlet", "");
  reportIntermediateParameter("cf_0",       sp().cf_0_, "Expected wall friction coefficient at the inlet");
  reportIntermediateParameter("utau_0",     sp().utau_0_, "Friction velocity at the inlet", "m/s");
  reportIntermediateParameter("theta_0",    sp().theta0_, "Momentum thickness at the inlet", "m");
  reportIntermediateParameter("delta99_0",  sp().delta99_0_, "Boundary layer thickness at the inlet", "m");
  reportIntermediateParameter("L",          sp().L_, "Domain length", "m");
  reportIntermediateParameter("Rex_e",      sp().Rex_e_, "Reynolds number with turbulent running length at the end of the domain", "");
  reportIntermediateParameter("ypfac_ref",  sp().ypfac_ref_, "yplus factor (y+/y, computed from friction coefficient at the inlet)");
  reportIntermediateParameter("H",          sp().H_, "height of the domain");
  reportIntermediateParameter("Hbytheta0",  sp().H_/sp().theta0_, "height of the domain, divided by initial BL thickness");
  reportIntermediateParameter("W",          sp().W_, "width of the domain");
  reportIntermediateParameter("Wbytheta0",  sp().W_/sp().theta0_, "width of the domain, divided by initial BL thickness");
  reportIntermediateParameter("Lbytheta0",  sp().L_/sp().theta0_, "length of the domain, divided by initial BL thickness");
  reportIntermediateParameter("deltaywall_ref", sp().deltaywall_ref_, "near-wall grid spacing");
  reportIntermediateParameter("gradl",      sp().gradl_, "near-wall layer block grading");
  reportIntermediateParameter("y_final",    sp().y_final_, "near-wall layer block height (clipped to 0.9*H)");
  reportIntermediateParameter("gradh",      sp().gradh_, "required vertical grid stretching");
  reportIntermediateParameter("nax",        sp().nax_, "number of cells in axial direction along the plate");
  reportIntermediateParameter("nlat",       sp().nlat_, "number of cells in lateral direction");
  reportIntermediateParameter("T",          sp().T_, "flow-through time");
}

void FlatPlateBL::createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& progress)
{
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");

  
  std::map<int, Point> pts = {
      {0, 	vec3( 0,        0.,               0)},
      {1, 	vec3( sp().L_,  0.,               0)},
      {10, 	vec3( 0,        sp().y_final_,    0)},
      {11, 	vec3( sp().L_,  sp().y_final_,    0)},
      {2, 	vec3( sp().L_,  sp().H_,          0)},
      {3, 	vec3( 0,        sp().H_,          0)}
      }
  ;
  
  // create patches
  Patch& in= 	bmd->addPatch(sp().in_, new Patch());
//  Patch& out= 	bmd->addPatch(out_, new Patch());
//   Patch& top= 	bmd->addPatch(top_, new Patch(/*"symmetryPlane"*/));
  Patch& out_top= 	bmd->addPatch(sp().out_top_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  std::string side_type="cyclic";
  if (p().mesh.twod) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch(sp().cycl_prefix_, new Patch(side_type));
  
  arma::mat vH=vec3(0, 0, sp().W_);

#define PTS(a,b,c,d) \
  P_8(pts[a], pts[b], pts[c], pts[d], \
      pts[a]+vH, pts[b]+vH, pts[c]+vH, pts[d]+vH)

  {
    Block& bl = bmd->addBlock
    (  
      new Block(
        PTS(10,11,2,3),
        sp().nax_, p().mesh.nh-p().mesh.nl, sp().nlat_,
        { 1, sp().gradh_, 1. }
      )
    );
    
    in.addFace(bl.face("0473"));    
    out_top.addFace(bl.face("1265"));
    out_top.addFace(bl.face("2376"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
  }
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,11,10),
        sp().nax_, p().mesh.nl, sp().nlat_,
        { 1, sp().gradl_, 1. }
      )
    );
    
    in.addFace(bl.face("0473"));    
    out_top.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
  }

  cycl_side.appendPatch(cycl_side_0);
  cycl_side.appendPatch(cycl_side_1);
  
  cm.insert(bmd.release());

  cm.createOnDisk(executionPath());
  cm.executeCommand(executionPath(), "blockMesh");
//   cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite") );
}

void FlatPlateBL::createInflowBC(insight::OpenFOAMCase& cm, const OFDictData::dict& boundaryDict) const
{
  {
    boost::filesystem::path inlet_velocity_profile_tabfile(  executionPath() / "inflow_velocity.dat");
    {
        std::ofstream f(inlet_velocity_profile_tabfile.c_str());
        f<<" 0.0 0.0 0.0 0.0"<<endl;
      
        int n=20;
        for (int i=1; i<n; i++)
        {
            double eta=double(i)/double(n-1);
            
            double UByUinf = 
                //2.*eta - 2.*pow(eta,3) + pow(eta,4);
                pow(eta, 1./7.);
                
            f<<(sp().delta99_0_*eta)<<" "<<(sp().uinf_*UByUinf)<<" 0.0 0.0"<<endl;
        }

        if (sp().H_>sp().delta99_0_)
        {
            f<<sp().H_<<" "<<sp().uinf_<<" 0.0 0.0"<<endl;
        }
    }
    
    VelocityInletBC::Parameters inflow_velocity;
    VelocityInletBC::Parameters::velocity_type::fielddata_linearProfile_type umean_data;

    // mean value profile
    umean_data.values.resize(1);
    umean_data.values[0].time=0;
    umean_data.values[0].profile->setOriginalFilePath(inlet_velocity_profile_tabfile); // without path! otherwise problems after case copying!
    
    umean_data.p0=vec3(0,0,0);      
    umean_data.ep=vec3(0,1,0);    
      
    inflow_velocity.velocity.fielddata=umean_data;
    
    cm.insert(new VelocityInletBC(cm, sp().in_, boundaryDict, inflow_velocity));
  }
}

void FlatPlateBL::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& progress)
{
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
  

  if (const auto *steady
        = boost::get<Parameters::run_type::regime_steady_type>(&p().run.regime))
  {
    cm.insert(new steadyIncompressibleNumerics(cm, steadyIncompressibleNumerics::Parameters()
      .set_checkResiduals(false) // don't stop earlier since averaging should be completed
      .set_Uinternal(vec3(sp().uinf_,0,0))
      .set_endTime(sp().end_)
      .set_np(np())
      .set_decompWeights(vec3(2,1,0))
      .set_decompositionMethod(FVNumerics::Parameters::decompositionMethod_type::hierarchical)
    ));
  } 
  else if (const auto *unsteady
        = boost::get<Parameters::run_type::regime_unsteady_type>(&p().run.regime))
  {
    cm.insert( new unsteadyIncompressibleNumerics(cm, unsteadyIncompressibleNumerics::Parameters()
      .set_LESfilteredConvection(p().run.filteredconvection)
      .set_Uinternal(vec3(p().operation.uinf, 0, 0))
//      .set_maxDeltaT(0.25*T_)

      .set_time_integration(
         unsteadyIncompressibleNumerics::Parameters::time_integration_type()
          .set_momentumPredictor(true)
          .set_timestep_control(PIMPLESettings::Parameters::timestep_control_adjust_type(
                                  0.9, // maxCo
                                  0.25*sp().T_ // maxDeltaT
                                ) )
          .set_pressure_velocity_coupling(
               PIMPLESettings::Parameters::pressure_velocity_coupling_PISO_type()
          )
       )
      .set_deltaT( 0.25 * sp().L_/double(sp().nax_) / sp().uinf_ )
      .set_endTime(sp().end_)

      .set_writeControl(FVNumerics::Parameters::writeControl_type::adjustableRunTime)
      .set_writeInterval(0.25*sp().T_)
      .set_decompositionMethod(FVNumerics::Parameters::decompositionMethod_type::hierarchical)
      .set_np(np())
      .set_decompWeights(vec3(2,1,0))
    ));
  }
  cm.insert(new extendedForces(cm, extendedForces::Parameters()
    .set_patches( list_of<string>("walls") )
  ));
  
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_fields(list_of<std::string>("p")("U")("pressureForce")("viscousForce"))
    .set_name("zzzaveraging") // shall be last FO in list
    .set_timeStart(sp().avgStart_)
  ));
  
//   std::vector<arma::mat> plocs;
//   for(size_t i=0; i<sec_locs_.size(); i++)
//   {
//     plocs.push_back(vec3(sec_locs_[i]*L, 0.5*W_, ));
//   }
//   cm.insert(new probes(cm, probes::Parameters()
//     .set_fields( list_of("U")("p") )
//     .set_probeLocations( list_of 
//       vec3()
//     )
//   ));
  
//   if (p.getBool("evaluation/eval2"))
//   {
//     cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
//       .set_name_prefix("tpc_interior")
//       .set_R(0.5*H)
//       .set_x(0.0) // middle x==0!
//       .set_z(-0.49*B)
//       .set_axSpan(0.5*L)
//       .set_tanSpan(0.45*B)
//       .set_timeStart( (inittime+meantime)*T_ )
//     ));
//   }

//   const SelectableSubsetParameter& tp = p.get<SelectableSubsetParameter>("mesh/tripping");
//   if (tp.selection()=="drag")
  
  cm.insert(new singlePhaseTransportProperties(
              cm,
              singlePhaseTransportProperties::Parameters().set_nu(p().fluid.nu)
              ));
  
//   cm.insert(new VelocityInletBC(cm, in_, boundaryDict, VelocityInletBC::Parameters()
//     .set_velocity(FieldData(vec3(uinf_,0,0)))
//     .set_turbulence(uniformIntensityAndLengthScale(0.005, 0.1*H_))
//   ) );

  createInflowBC(cm, boundaryDict);
  
//  if (patchExists(boundaryDict, approach_)) // make possible to evaluate old cases without approach patch
//    cm.insert(new SimpleBC(cm, approach_, boundaryDict, "symmetryPlane") );
  //  leave approach as wall to produce laminar BL upstream
  
//   cm.insert(new SuctionInletBC(cm, top_, boundaryDict, SuctionInletBC::Parameters()
//     .set_pressure(0.0)
//   ));
//   cm.insert(new SimpleBC(cm, top_, boundaryDict, "symmetryPlane") );
//   cm.insert(new PressureOutletBC(cm, top_, boundaryDict, PressureOutletBC::Parameters()
//     .set_pressure(0.0)
//   ));
// 
//   cm.insert(new PressureOutletBC(cm, out_, boundaryDict, PressureOutletBC::Parameters()
//     .set_pressure(0.0)
//   ));
  
  cm.insert(new PressureOutletBC(cm, sp().out_top_, boundaryDict, PressureOutletBC::Parameters()
    .set_behaviour( PressureOutletBC::Parameters::behaviour_uniform_type(
       FieldData::Parameters()
        .set_fielddata(FieldData::Parameters::fielddata_uniformSteady_type(vec1(0.0)))
      ))
    .set_prohibitInflow(false)
  ));
  
  if (p().mesh.twod)
    cm.insert(new SimpleBC(cm, sp().cycl_prefix_, boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, sp().cycl_prefix_, boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p().fluid.turbulenceModel);
}

void FlatPlateBL::evaluateAtSection
(
  OpenFOAMCase& cm,
  ResultSetPtr results, double x, int /*i*/,
  const Interpolator& cfi,
  const std::string& UMeanName,
  const std::string& RFieldName,
  const FlatPlateBL::Parameters::eval_type::bc_extractsections_default_type* extract_section
)
{
  double xByL= x/sp().L_;
  string prefix="section";
  if (extract_section)
    prefix=extract_section->name_prefix;
  string title=prefix+"__xByL_" + str(format("%04.1f") % xByL);
  replace_all(title, ".", "_");
  
  TabularResult *table=nullptr, *table2=nullptr;
  TabularResult::Row *thisctrow=nullptr, *thisctrow2=nullptr;
  if (!extract_section)
  {
    table = &(results->get<TabularResult>("tableCoefficients"));
    thisctrow = &(table->appendRow());
    table->setCellByName(*thisctrow, "x/L", xByL);

    table2 = &(results->get<TabularResult>("tableValues"));
    thisctrow2 = &(table2->appendRow());
    table2->setCellByName(*thisctrow2, "x/L", xByL);
  }
//   //estimate delta2
//   double delta2_est = 0.5*FlatPlateBL::cw(uinf_*x/nu)*x;
    
  boost::ptr_vector<sampleOps::set> sets;
  
  double 
    miny=0.99*sp().deltaywall_ref_,
    maxy=std::min(1.5*sp().delta99_0_, sp().H_ - sp().deltaywall_ref_);
    
  arma::mat pts=exp(linspace(log(miny), log(maxy), 101)) * vec3(0,1,0).t();
  pts.col(0)+=x;
  pts.col(2)+=0.01*sp().W_;
  
  sets.push_back(new sampleOps::linearAveragedPolyLine(sampleOps::linearAveragedPolyLine::Parameters()
    .set_points( pts )
    .set_dir1(vec3(1,0,0))
    .set_dir2(vec3(0,0,0.98*sp().W_))
    .set_nd1(1)
    .set_nd2(sp().n_hom_avg_)
    .set_name("radial")
  ));
  
  sample(cm, executionPath(), 
     list_of<std::string>("p")(UMeanName)(RFieldName)("k")("omega")("epsilon")("nut"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data =
    sampleOps::findSet<sampleOps::linearAveragedPolyLine>(sets, "radial").readSamples(cm, executionPath(), &cd);
  arma::mat y=data.col(0)+sp().deltaywall_ref_;

  double tauw=as_scalar( 0.5*cfi(x) * pow(sp().uinf_,2) );
  double utau=sqrt(tauw);
  double ypByy=utau/p().fluid.nu;
  arma::mat yplus=y*ypByy;
    
  arma::uword cU=cd[UMeanName].col;
  arma::mat upaxial(join_rows(yplus, data.col(cU)/utau));
  arma::mat upwallnormal(join_rows(yplus, data.col(cU+1)/utau));
  arma::mat upspanwise(join_rows(yplus, data.col(cU+2)/utau));
  
  arma::mat uByUinf=join_rows(y, data.col(cU)/sp().uinf_);
  arma::mat delta123 = integrateDelta123( uByUinf );
  double delta99 = searchDelta99( uByUinf );
  
  double Re_theta=sp().uinf_*delta123(1)/p().fluid.nu;

  if (table)
  {
    table->setCellByName(*thisctrow, "$Re_\\theta$", Re_theta);
    table->setCellByName(*thisctrow, "delta1+", delta123(0)*ypByy);
    table->setCellByName(*thisctrow, "delta2+", delta123(1)*ypByy);
    table->setCellByName(*thisctrow, "delta3+", delta123(2)*ypByy);
    table->setCellByName(*thisctrow, "delta99+", delta99*ypByy );
  }
    
  if (table2)
  {
    table2->setCellByName(*thisctrow2, "delta1", delta123(0));
    table2->setCellByName(*thisctrow2, "delta2", delta123(1));
    table2->setCellByName(*thisctrow2, "delta3", delta123(2));
    table2->setCellByName(*thisctrow2, "delta99", delta99 );
    table2->setCellByName(*thisctrow2, "tauw", tauw);
    table2->setCellByName(*thisctrow2, "utau", utau);
  }
  
  // Mean velocity profiles
  {
    if (extract_section)
    {
      arma::mat u=join_rows(y, data.cols(cU, cU+2));
      u.save( (
	executionPath() /
	str( format("umean_vs_y_%s_x%05.2f.txt") % extract_section->name_prefix % x )
      ).string(), raw_ascii);
    }
    
    double maxU=1.1*sp().uinf_;
    
    arma::mat delta1pc(delta123(0)*ones(2,2)*ypByy);
    delta1pc(0,1)=0.; delta1pc(1,1)=maxU;
    
    arma::mat delta2pc(delta123(1)*ones(2,2)*ypByy);
    delta2pc(0,1)=0.; delta2pc(1,1)=maxU;

    arma::mat delta3pc(delta123(2)*ones(2,2)*ypByy);
    delta3pc(0,1)=0.; delta3pc(1,1)=maxU;
    
    arma::mat delta99pc(delta99*ones(2,2)*ypByy);
    delta99pc(0,1)=0.; delta99pc(1,1)=maxU;
    
    arma::mat visclayer=linspace(0, 10, 10), loglayer=linspace(30,300,2);
    visclayer=join_rows( visclayer, visclayer );
    loglayer=join_rows( loglayer, (1./0.41)*log(loglayer)/log(M_E) + 5. );

    addPlot
    (
      results, executionPath(), "chartMeanVelocity_"+title,
      "$y^+$", "$\\langle U^+ \\rangle$",
      {
        PlotCurve(upaxial, "Up", "w l lt 1 lc 1 lw 4 t 'Axial'"),
        PlotCurve(upspanwise, "Up", "w l lt 1 lc 2 lw 4 t 'Spanwise'"),
        PlotCurve(upwallnormal, "Wp", "w l lt 1 lc 3 lw 4 t 'Wall normal'"),
        PlotCurve(delta1pc, "delta1p", "w l lt 2 lc 4 lw 1 t '$\\delta_1^+$'"),
        PlotCurve(delta2pc, "delta2p", "w l lt 3 lc 4 lw 1 t '$\\delta_2^+$'"),
        PlotCurve(delta3pc, "delta3p", "w l lt 4 lc 4 lw 1 t '$\\delta_3^+$'"),
        PlotCurve(delta99pc, "delta99p", "w l lt 5 lc 4 lw 1 t '$\\delta_{99}^+$'"),
	
        PlotCurve(visclayer, "visc", "w l lt 2 lc 5 lw 2 t 'Viscous Layer'"),
        PlotCurve(loglayer, "log", "w l lt 3 lc 5 lw 2 t 'Log Layer'")
      },
      str(format("Wall normal profiles of averaged velocities at x/L=%g (Re_theta=%g)") % xByL % Re_theta),
     
      str( format("set key top left reverse Left; set logscale x; set xrange [:%g]; set yrange [0:%g];") 
                % (ypByy*std::max(sp().theta0_, 10.*delta123(1)))
		% (maxU/utau) 
	 )
      
    );
  }

  // Reynolds stress profiles
  std::cout<<"index of "<<RFieldName<<": "<<cd[RFieldName].col<<"."<<std::endl;
  arma::uword cR=cd[RFieldName].col;
  arma::mat Rpuu(join_rows(yplus, data.col(cR)/pow(utau,2)));
  arma::mat Rpvv(join_rows(yplus, data.col(cR+3)/pow(utau,2)));
  arma::mat Rpww(join_rows(yplus, data.col(cR+5)/pow(utau,2)));
  {
    arma::mat R_vs_y=join_rows(y, data.cols(cR, cR+5));
    
    if (extract_section)
    {
      R_vs_y.save( (
	executionPath() /
	str( format("R_vs_y_%s_x%05.2f.txt") % extract_section->name_prefix % x )
      ).string(), raw_ascii);
    }
    
    double maxRp=1.1*as_scalar(arma::max(arma::max(R_vs_y.cols(1,6))))/pow(utau,2);
    
    addPlot
    (
      results, executionPath(), "chartReynoldsStress_"+title,
      "$y^+$", "$\\langle R^+ \\rangle$",
      {
        PlotCurve(Rpuu, "Rpuu", "w l lt 1 lc 1 lw 4 t 'Axial'"),
        PlotCurve(Rpvv, "Rpvv", "w l lt 1 lc 2 lw 4 t 'Wall normal'"),
        PlotCurve(Rpww, "Rpww", "w l lt 1 lc 3 lw 4 t 'Spanwise'")
      },
      "Wall normal profiles of Reynolds stresses at x/L=" + str(format("%g")%xByL),
     
      str( format("set xrange [:%g]; set yrange [0:%g];") 
                % (ypByy*std::max(sp().theta0_, 10.*delta123(1)))
		% (maxRp) 
	 )
      
    );
  }
  
  if (cd.find("k")!=cd.end())
  {    
    arma::uword ck=cd["k"].col;
    
    arma::mat kp_vs_yp=join_rows(yplus, (data.col(ck)/pow(utau,2)) + 0.5*(Rpuu.col(1)+Rpvv.col(1)+Rpww.col(1)));
    
    if (extract_section)
    {
      arma::mat k_vs_y=join_rows( y, kp_vs_yp.col(1)*pow(utau,2) );
      k_vs_y.save( (
	executionPath() /
	str( format("k_vs_y_%s_x%05.2f.txt") % extract_section->name_prefix % x )
      ).string(), raw_ascii);
    }
    
    addPlot
    (
      results, executionPath(), "chartTKE_"+title,
      "$y^+$", "$\\langle k^+ \\rangle$",
      {
       PlotCurve(kp_vs_yp, "kp", "w l lt 2 lc 1 lw 1 not")
      },
      "Wall normal profile of total turbulent kinetic energy at x/L=" + str(format("%g")%xByL) /*,
      "set logscale x"*/
    );
  }
}

insight::ResultSetPtr FlatPlateBL::evaluateResults(insight::OpenFOAMCase& cm, ProgressDisplayer& progress)
{
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, progress);
  
  std::string RFieldName="UPrime2Mean";
  std::string UMeanName="UMean";
  if ( const RASModel *rm = cm.get<RASModel>(".*") )
  {
    std::cout<<"Case included RASModel "<<rm->name()<<". Computing R field"<<std::endl;
    calcR(cm, executionPath());
    RFieldName="R";
    UMeanName="U";
  }
  
  if (!((cm.OFversion()>160) && (cm.OFversion()<200)))
  {
    calcLambda2(cm, executionPath());
//     cm.executeCommand(executionPath(), "Lambda2", list_of("-latestTime"));
    
    double lambda2=1e5;
    
    std::string name="figVortexStructures";
    std::string fname=name+".png";
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	"cbi=loadOFCase('.')\n"
        +str(format("L=%g\n") % sp().L_)+
	"prepareSnapshots()\n"

	"eb=extractPatches(cbi, '^(wall|approach|oldInternalFaces)')\n"
	"Show(eb)\n"
	"displayContour(eb, 'viscousForce', arrayType='CELL_DATA', barpos=[0.75,0.15], barorient=0)\n"

	"interior=extractInterior(cbi)\n"
	"contour1 = Contour(Input=interior)\n"
	"contour1.PointMergeMethod = 'Uniform Binning'\n"
	"contour1.ContourBy = ['POINTS', 'Lambda2']\n"
	+str(format("contour1.Isosurfaces = [%g]\n")%lambda2)+
	"displaySolid(contour1)\n"

	"setCam([-L,0.15*L,0.15*L], [0.5*L,0,0], [0,1,0], 0.1*L)\n"
	"WriteImage('"+fname+"')\n"
      )
    );
    results->insert(name,
      std::unique_ptr<Image>(new Image
      (
      executionPath(), fname, 
      str(format("Vortex structures, vizualized as isosurfaces of $\\lambda_2=%g$")%(-lambda2)), ""
    )));  
  }

  // Wall friction coefficient
  arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), sp().nax_);
    
  arma::mat Cf_vs_x(join_rows(
      wallforce.col(0), 
      wallforce.col(1)/(0.5*pow(sp().uinf_,2))
    ));
  Cf_vs_x.save( (executionPath()/"Cf_vs_x.txt").string(), arma_ascii);
  Interpolator Cf_vs_x_i(Cf_vs_x);

#ifdef HAS_REFDATA
  arma::mat Cfexp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/cf_vs_x");
//  Interpolator Cfexp_vs_x_i(Cfexp_vs_x);
#endif

  
  addPlot
  (
    results, executionPath(), "chartMeanWallFriction",
    "x [m]", "$\\langle C_f \\rangle$",
    {
      PlotCurve(Cf_vs_x, "cfd", "w l lt 1 lc 2 lw 2 t 'CFD'")
#ifdef HAS_REFDATA
          ,
      PlotCurve(Cfexp_vs_x, "ref", "w p lt 2 lc 2 t 'Wieghardt 1951 (u=17.8m/s)'")
#endif
    },
    "Axial profile of wall friction coefficient"
  );    
  
  results->insert("tableCoefficients",
    std::unique_ptr<TabularResult>(new TabularResult
    (
      list_of("x/L")("$Re_\\theta$")("delta1+")("delta2+")("delta3+")("delta99+"),
      arma::mat(),
      "Boundary layer properties along the plate (normalized)", "", ""
  )));
  
  results->insert("tableValues",
    std::unique_ptr<TabularResult>(new TabularResult
    (
      list_of("x/L")("delta1")("delta2")("delta3")("delta99")("tauw")("utau"),
      arma::mat(),
      "Boundary layer properties along the plate (not normalized)", "", ""
  )));
  
  for (size_t i=0; i<sp().sec_locs_.size(); i++)
  {
    evaluateAtSection
    (
      cm, 
      results, 
      sp().sec_locs_[i] * sp().L_,
      i, 
      Cf_vs_x_i, 
      UMeanName, 
      RFieldName
    );
  }
  
  for (const auto& es: p().eval.bc_extractsections)
  {
    evaluateAtSection
    (
      cm, 
      results, 
      es.x, 
      -1, 
      Cf_vs_x_i, 
      UMeanName, 
      RFieldName, 
      &es
    );
  }

  {  
#ifdef HAS_REFDATA
    arma::mat delta1exp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/delta1_vs_x");
    arma::mat delta2exp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/delta2_vs_x");
    arma::mat delta3exp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/delta3_vs_x");
#endif
    
    const insight::TabularResult& tabcoeffs=results->get<TabularResult>("tableCoefficients");
    const insight::TabularResult& tabvals=results->get<TabularResult>("tableValues");
    arma::mat ctd=tabvals.toMat();
    
    addPlot
    (
      results, executionPath(), "chartDelta",
      "x [m]", "$\\delta$ [m]",
      {
#ifdef HAS_REFDATA
        PlotCurve(delta1exp_vs_x, "delta1ref", "w p lt 1 lc 1 t '$\\delta_1$ (Wieghardt 1951, u=17.8m/s)'"),
        PlotCurve(delta2exp_vs_x, "delta2ref", "w p lt 2 lc 3 t '$\\delta_2$ (Wieghardt 1951, u=17.8m/s)'"),
        PlotCurve(delta3exp_vs_x, "delta3ref", "w p lt 3 lc 4 t '$\\delta_3$ (Wieghardt 1951, u=17.8m/s)'"),
#endif

        PlotCurve(arma::mat(join_rows(sp().L_*ctd.col(0), tabvals.getColByName("delta1"))), "delta1", "w l lt 1 lc 1 lw 2 t '$\\delta_1$'"),
        PlotCurve(arma::mat(join_rows(sp().L_*ctd.col(0), tabvals.getColByName("delta2"))), "delta2", "w l lt 1 lc 3 lw 2 t '$\\delta_2$'"),
        PlotCurve(arma::mat(join_rows(sp().L_*ctd.col(0), tabvals.getColByName("delta3"))), "delta3", "w l lt 1 lc 4 lw 2 t '$\\delta_3$'")
      },
      "Axial profile of boundary layer thickness",
      "set key top left reverse Left"
    );
    
    arma::mat Re_theta=tabcoeffs.getColByName("$Re_\\theta$");
    arma::mat xL=tabcoeffs.getColByName("x/L");
    arma::mat Rex=(sp().Rex_0_ + sp().uinf_ * xL * sp().L_ / p().fluid.nu)/1e5;
//    Interpolator delta2exp_vs_x_i(delta2exp_vs_x);
    addPlot
    (
      results, executionPath(), "chartRetheta",
      "$Re_x$ /$10^5$", "$Re_{\\theta}$",
      {
        PlotCurve(Rex, Re_theta, "Retheta", "w lp lt 1 lc 1 lw 2 t '$Re_{\\theta}$'"),
        PlotCurve("Rethetaanalytical", "0.037*(1e5*x)**(4./5.) w l lt 2 lc 1 t 'Analytical (Cengel)'")
      },
      "Axial profile of boundary layer thickness",
      "set key top left reverse Left"
    );

    addPlot
    (
      results, executionPath(), "chartDelta99",
      "x [m]", "$\\delta$ [m]",
      {
        PlotCurve(sp().L_*ctd.col(0), tabvals.getColByName("delta1"), "delta1", "w l lt 1 lc 1 lw 2 t '$\\delta_1$'"),
        PlotCurve(sp().L_*ctd.col(0), tabvals.getColByName("delta2"), "delta2", "w l lt 1 lc 3 lw 2 t '$\\delta_2$'"),
        PlotCurve(sp().L_*ctd.col(0), tabvals.getColByName("delta3"), "delta3", "w l lt 1 lc 4 lw 2 t '$\\delta_3$'"),
        PlotCurve(sp().L_*ctd.col(0), tabvals.getColByName("delta99"), "delta99", "w l lt 1 lc 5 lw 2 t '$\\delta_{99}$'")
      },
      "Axial profile of boundary layer thickness",
      "set key top left reverse Left"
    );

  }
  
  
  
  return results;
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
  return 2.*pow( (0.41/log(Re)) * G( log(Re), 2.*log(0.41)+0.41*(Cplus-3.) ), 2); // Schlichting eq. (18.99), Kap. 18.2.5
}

double FlatPlateBL::cf(double Rex, double Cplus, cf_method method)
{
  switch (method)
  {
    case cf_method_Schlichting:
    {
      struct Obj: public Objective1D
      {
	double Rex, Cplus;
	virtual double operator()(double gamma) const 
	{ 
	  return (1./gamma) -(1./0.41)*log(gamma*gamma*Rex)  // Schlichting, Eq. (18.93), Kap. 18.2.5
	    - Cplus - (1./0.41)*(2.*0.55-log(3.78)); 
	}
      } obj;
      obj.Rex=Rex;
      obj.Cplus=Cplus;
      double gamma=nonlinearSolve1D(obj, 1e-7, 10.);
      return 2.*gamma*gamma;
    }
    
    case cf_method_Cengel:
      return 0.059*pow(Rex, -1./5.);
      
    default:
      throw insight::Exception("Unknown method for computation of cf!");
  }
  
  return FP_NAN;      
}

double FlatPlateBL::Redelta99(double Rex, Redelta99_method method)
{
  switch (method)
  {
    case Redelta99_method_Schlichting:
    {
      double Cplus=5., Cquer=2.1;
      double D=(log(2*0.41)+0.41*(Cplus+Cquer));
      return 0.14*(Rex/log(Rex))*G(log(Rex), D); // Schlichting eq. (2.12), Kap. 2.3
    } 
    case Redelta99_method_Cengel:
      return 0.38*pow(Rex, 4./5.);
      
    default:
      throw insight::Exception("Unknown method for computation of Redelta2!");
  }
  
  return FP_NAN;
}

double FlatPlateBL::Redelta2(double Rex, Redelta2_method method)
{
  switch (method)
  {
    case Redelta2_method_Schlichting:
    {
      return 0.5*cw(Rex)*Rex; // Schlichting eq. (18.100), Kap. 18.2.5
      break;
    }
    
    case Redelta2_method_Cengel:
    {
      return 0.037*pow(Rex, 4./5.);
      break;
    }
      
    default:
      throw insight::Exception("Unknown method for computation of Redelta2!");
  }

  return FP_NAN;
}

double FlatPlateBL::Rex(double Redelta2, Redelta2_method method)
{
  switch (method)
  {
    case Redelta2_method_Schlichting:
    {
        struct Obj : public Objective1D
        {
            double Redelta2;
            virtual double operator()(double Rex) const 
            { 
            return FlatPlateBL::Redelta2(Rex)-Redelta2; 
            }
            
        } o;
        o.Redelta2=Redelta2;
        return nonlinearSolve1D(o, o.Redelta2, 1e6);
    } 
    
    case Redelta2_method_Cengel:
    {
      return pow(Redelta2/0.037, 5./4.);
    }
      
    default:
      throw insight::Exception("Unknown method for computation of Rex!");
  }

  return FP_NAN;
}


double FlatPlateBL::Retau(double Redelta2)
{
    return 1.13*pow(Redelta2, 0.843); // aus Schlatter, Örlü http://dx.doi.org/doi:doi:10.1017/S0022112010003113
}


arma::mat FlatPlateBL::integrateDelta123(const arma::mat& uByUinf_vs_y)
{
  arma::mat delta(zeros(3));
  
  arma::mat x = uByUinf_vs_y.col(0);
  arma::mat y = uByUinf_vs_y.col(1);
  for (arma::uword i=0; i<y.n_elem; i++) y(i)=std::max(0., std::min(1., y(i)));
  //arma::mat y = clamp(uByUinf_vs_y.col(1), 0., 1);

  arma::mat y1, y2, y3;
  y1 = (1. - y);
  y2 = y % (1. - y);
  y3 = y % (1. - pow(y,2));
  
  if (fabs(x(0)) > 1e-10)
  {
    delta(0) += 0.5*y1(0) * x(0);
    delta(1) += 0.5*y2(0) * x(0);
    delta(2) += 0.5*y3(0) * x(0);
  }
  
  for (arma::uword i=0; i<uByUinf_vs_y.n_rows-1; i++)
  {
    delta(0) += 0.5*( y1(i) + y1(i+1) ) * ( x(i+1) - x(i) );
    delta(1) += 0.5*( y2(i) + y2(i+1) ) * ( x(i+1) - x(i) );
    delta(2) += 0.5*( y3(i) + y3(i+1) ) * ( x(i+1) - x(i) );
  }
  
  return delta;
}

double FlatPlateBL::searchDelta99(const arma::mat& uByUinf_vs_y)
{
  arma::uword i=0;
  for (i=0; i<uByUinf_vs_y.n_rows; i++)
  {
    if (uByUinf_vs_y(i,1)>=0.99) break;
  }
  return uByUinf_vs_y(std::min(i, arma::uword(uByUinf_vs_y.n_rows-1)),0);
}




}
