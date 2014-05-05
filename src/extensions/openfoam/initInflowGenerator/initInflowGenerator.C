/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Application
    yPlusRAS

Description
    Calculates and reports yPlus for all wall patches, for the specified times.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "OFstream.H"
#include "transformField.H"
#include "transformGeometricField.H"
#include "fixedGradientFvPatchFields.H"
#include "addToRunTimeSelectionTable.H"
#include "inflowGeneratorBaseFvPatchVectorField.H"
#include "wallDist.H"
#include "interpolationTable.H"

#include <boost/concept_check.hpp>
#include <boost/assign.hpp>

#include "pipe.h"
#include "channel.h"
#include "refdata.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace insight;


namespace insight
{
  
struct FlowProps
{
  Foam::vector Ubulk_;
  double delta_;
  double utau_;
  double Retau_;
  double Re_;
};

/**
 * abstract base class for providing the length scale L_delta=f(y_delta)
 */
class LengthScaleModel
{
public:
  declareFactoryTable(LengthScaleModel, Foam::dictionary);  

public:
  declareType("LengthScaleModel");
  
  virtual ~LengthScaleModel() {}
  /**
   * gets y, returns the absolute length scale
   */
  virtual Foam::symmTensor operator()(const FlowProps& flow, double y) const =0;
};

defineType(LengthScaleModel);
defineFactoryTable(LengthScaleModel, Foam::dictionary);

/**
 * abstract base class for providing the mean velocity profile U_mean/utau = f(y+)
 */
class MeanVelocityModel
{
public:
  declareFactoryTable(MeanVelocityModel, Foam::dictionary);  
  
public:
  declareType("MeanVelocityModel");
  
  virtual ~MeanVelocityModel() {}
  /**
   * gets y, return the absolute mean velocity
   */
  virtual Foam::vector operator()(const FlowProps& flow, double y) const =0;
};

defineType(MeanVelocityModel);
defineFactoryTable(MeanVelocityModel, Foam::dictionary);

/**
 * abstract base class for providing the reynolds stress profile RMS/sqr(utau) = f(y+)
 */
class ReynoldsStressModel
{
public:
  declareFactoryTable(ReynoldsStressModel, Foam::dictionary);  
  
public:
  declareType("ReynoldsStressModel");
  
  virtual ~ReynoldsStressModel() {}
  /**
   * gets y, return the absolute RMS
   */
  virtual Foam::symmTensor operator()(const FlowProps& flow, double y) const =0;
};

defineType(ReynoldsStressModel);
defineFactoryTable(ReynoldsStressModel, Foam::dictionary);

//=============================================================================
//========================= Mean Velocity Model Implementations ================
//=============================================================================


class PowerLawMeanVelocity
: public MeanVelocityModel
{
  scalar n_;
  
public:
  declareType("PowerLawMeanVelocity");
  
  PowerLawMeanVelocity(const Foam::dictionary& d)
  : n_(readScalar(d.lookup("n")))
  {
    Info<<"Initializing mean velocity field with power law (exponent = 1/"<<n_<<")"<<endl;
  }
  
  virtual Foam::vector operator()(const FlowProps& flow, double y) const
  {
    return flow.Ubulk_*::pow(y/flow.delta_, 1./n_);
  }
};

defineType(PowerLawMeanVelocity);
addToFactoryTable(MeanVelocityModel, PowerLawMeanVelocity, Foam::dictionary);

//=============================================================================
//========================= Length Scale Model Implementations ================
//=============================================================================

class LengthScaleFit
{
protected:
  double c0_, c1_, c2_, c3_;
  double minL_;
  
public:
  LengthScaleFit(const Foam::dictionary& d)
  : c0_(readScalar(d.lookup("c0"))),
    c1_(readScalar(d.lookup("c1"))),
    c2_(readScalar(d.lookup("c2"))),
    c3_(readScalar(d.lookup("c3"))),
    minL_(d.lookupOrDefault<scalar>("minL", 1e-6))
  {
  }
  
  /**
   * return Ldelta(ydelta)
   */
  inline double operator()(scalar ydelta) const
  {
    return max(minL_, c0_*::pow(ydelta, c2_) + c1_*::pow(ydelta, c3_));
  }
  
  void writeCoeffs(Foam::Ostream& os) const
  {
    os << "[c0="<<c0_
    << " c1="<<c1_
    << " c2="<<c2_
    << " c3="<<c3_<<"]";
  }
};

class FittedIsotropicLengthScaleModel
: public LengthScaleModel
{
  LengthScaleFit L_;
  
public:
  declareType("FittedIsotropicLengthScaleModel");
  
  FittedIsotropicLengthScaleModel(const Foam::dictionary& d)
  : L_(d)
  {
    Info<<"Initializing length scale field with isotropic length scaler according to fit ";
    L_.writeCoeffs(Info);
    Info<<endl;
  }
  
  virtual Foam::symmTensor operator()(const FlowProps& flow, scalar y) const
  {
    double L = flow.delta_ * L_(y/flow.delta_);
    
    return Foam::symmTensor(L, 0, 0, L, 0, L);
  }
};

defineType(FittedIsotropicLengthScaleModel);
addToFactoryTable(LengthScaleModel, FittedIsotropicLengthScaleModel, Foam::dictionary);


class FittedAnisotropicLengthScaleModel
: public LengthScaleModel
{
  LengthScaleFit Lx_, Ly_, Lz_;
public:
  declareType("FittedAnisotropicLengthScaleModel");
  
  FittedAnisotropicLengthScaleModel(const Foam::dictionary& d)
  : Lx_(d.subDict("x")),
    Ly_(d.subDict("y")),
    Lz_(d.subDict("z"))
  {
    Info<<"Initializing length scale field with anisotropic length scale according to fits: "<<endl;
    Info<<" x: "; Lx_.writeCoeffs(Info);Info<<endl;
    Info<<" y: "; Ly_.writeCoeffs(Info);Info<<endl;
    Info<<" z: "; Lz_.writeCoeffs(Info);Info<<endl;
  }
  
  virtual Foam::symmTensor operator()(const FlowProps& flow, scalar y) const
  {
    return flow.delta_*Foam::symmTensor(Lx_(y/flow.delta_), 0, 0, Ly_(y/flow.delta_), 0, Lz_(y/flow.delta_));
  }
};

defineType(FittedAnisotropicLengthScaleModel);
addToFactoryTable(LengthScaleModel, FittedAnisotropicLengthScaleModel, Foam::dictionary);

//=============================================================================
//=============================================================================
//=============================================================================

class WallLayerReynoldsStresses
: public ReynoldsStressModel,
  public interpolationTable<symmTensor>
{
  static std::vector<double> yp;
  static std::vector<double> Ruu;
  static std::vector<double> Rvv;
  static std::vector<double> Rww;
  
public:
  declareType("WallLayerReynoldsStresses");
  
  WallLayerReynoldsStresses(const Foam::dictionary&)
  {
    resize(yp.size());
    for (size_t i=0; i<yp.size(); i++)
    {
      UList<Tuple2<scalar, symmTensor> >::operator[](i).first()=yp[i];
      UList<Tuple2<scalar, symmTensor> >::operator[](i).second()=symmTensor(Ruu[i], 0, 0, Rvv[i], 0, Rww[i]);
    }
    outOfBounds(CLAMP);
  }

  virtual Foam::symmTensor operator()(const FlowProps& flow, double y) const
  {
    return interpolationTable<symmTensor>::operator()(y*flow.Retau_) * Foam::sqr(flow.Retau_);
  }
};

std::vector<double> WallLayerReynoldsStresses::yp=list_of<double>(0.0)(0.182483)(2.92006)(3.28802)(3.29234)(3.29433)(3.66262)(4.03091)(4.39887)(4.58468)(4.74988)(4.76018)(4.77048)(5.13877)(5.49909)(5.51803)(5.70384)(5.86738)(6.02925)(6.05318)(6.07977)(6.26558)(6.42147)(6.45139)(6.63719)(6.78943)(6.823)(6.97524)(7.00881)(7.19462)(7.30896)(7.34353)(7.38042)(7.52933)(7.56623)(7.71514)(7.75204)(7.93784)(8.08343)(8.12332)(8.30912)(8.45172)(8.49493)(8.589)(8.68074)(8.81968)(9.04903)(9.18797)(9.23483)(9.55626)(9.60312)(9.86871)(9.97141)(10.107)(10.1572)(10.5255)(10.6575)(10.8935)(10.9516)(10.9663)(11.2082)(11.2618)(11.63)(11.9412)(12.2456)(12.363)(12.5946)(12.8562)(13.0959)(13.5253)(13.8288)(13.9538)(14.4201)(14.9875)(15.2335)(15.291)(16.0632)(16.4494)(16.7492)(16.8768)(17.7288)(17.8886)(18.0243)(18.7013)(18.934)(19.1906)(19.5317)(20.0263)(20.5255)(20.6525)(21.1182)(21.3572)(21.8019)(22.0279)(22.1143)(22.9373)(23.1826)(23.4432)(23.7584)(24.0296)(24.8253)(25.1218)(25.2667)(25.402)(26.0316)(26.6508)(26.9078)(27.046)(27.1235)(28.2157)(28.4763)(28.5491)(28.6894)(29.3083)(30.1193)(30.1901)(30.3331)(30.7652)(31.8311)(31.8574)(31.9445)(32.1589)(33.1325)(33.655)(33.7697)(33.8019)(34.2254)(35.2963)(35.4123)(35.5007)(35.6274)(36.7758)(36.9373)(37.2375)(37.2704)(38.0512)(38.5784)(39.0627)(39.278)(39.5087)(40.4025)(40.8875)(40.9207)(40.9666)(42.0439)(42.4248)(42.7126)(42.7459)(43.8677)(44.0655)(44.3553)(44.5707)(45.4506)(45.5094)(45.7062)(46.3955)(47.2757)(47.3332)(47.3472)(48.2207)(48.9749)(48.9879)(49.1005)(50.0455)(50.4461)(50.7991)(50.9254)(51.8704)(52.0871)(52.6232)(52.7502)(53.6952)(53.7284)(54.2649)(54.3932)(55.3372)(55.3698)(56.0891)(56.218)(57.0108)(57.1617)(57.7308)(58.0432)(58.8346)(58.9865)(59.5549)(59.868)(60.4763)(60.811)(61.1969)(61.6932)(62.3001)(62.6359)(63.0211)(63.518)(63.9418)(64.4607)(64.8456)(65.3432)(65.766)(66.1024)(66.4876)(67.168)(67.5902)(67.9269)(68.3118)(68.9929)(69.2322)(69.7517)(70.1363)(70.8177)(71.0567)(71.5759)(71.7783)(72.4604)(72.8812)(73.4007)(73.6028)(74.2852)(74.7057)(75.0424)(75.4273)(76.11)(76.3477)(76.8669)(77.0693)(77.9352)(78.1722)(78.6914)(78.8938)(79.5775)(79.997)(80.5159)(80.5355)(81.4024)(81.8215)(82.1579)(82.3606)(83.2272)(83.6463)(83.9824)(84.0027)(85.0524)(85.4708)(85.8069)(85.8275)(86.8772)(87.1128)(87.6314)(87.6523)(88.702)(88.9377)(89.2734)(89.4768)(90.344)(90.7618)(91.0979)(91.3013)(92.1692)(92.5867)(92.9224)(92.9437)(93.8115)(94.4115)(94.7469)(94.7688)(95.6364)(96.236)(96.3889)(96.5933)(97.4612)(98.0605)(98.2134)(98.4181)(99.286)(99.7025);
std::vector<double> WallLayerReynoldsStresses::Ruu=list_of<double>(0.0)(0.012173)(0.467221)(0.528384)(0.529102)(0.529433)(0.711717)(0.894)(1.05807)(1.24029)(1.40229)(1.4124)(1.4225)(1.60479)(2.34903)(2.38817)(2.57038)(2.83193)(3.09081)(3.12909)(3.17161)(3.35383)(3.50671)(3.53605)(3.71826)(3.86756)(3.90048)(4.04978)(4.0827)(4.26492)(4.37705)(4.41095)(4.44713)(4.59317)(4.62935)(4.77538)(4.81157)(4.99378)(5.12252)(5.15779)(5.34)(5.47984)(5.52222)(5.61447)(5.70444)(5.77321)(5.88672)(6.02298)(6.06894)(6.22802)(6.25122)(6.38267)(6.43351)(6.5665)(6.61572)(6.79801)(6.85685)(6.96207)(6.99086)(6.9981)(7.11787)(7.14436)(7.32664)(7.39634)(7.46456)(7.49084)(7.54275)(7.60135)(7.65504)(7.75126)(7.81925)(7.83019)(7.87103)(7.92071)(7.94225)(7.94728)(7.89934)(7.87536)(7.85674)(7.84402)(7.75914)(7.74321)(7.7297)(7.62149)(7.58431)(7.55017)(7.5048)(7.43899)(7.36422)(7.3452)(7.27545)(7.23726)(7.16619)(7.13006)(7.11452)(6.96646)(6.93382)(6.89915)(6.85723)(6.82114)(6.71527)(6.67582)(6.65266)(6.63104)(6.53043)(6.43769)(6.3992)(6.37849)(6.36689)(6.22157)(6.19126)(6.18279)(6.16647)(6.09447)(6.0035)(5.99556)(5.97953)(5.93106)(5.78923)(5.78574)(5.77706)(5.7557)(5.6587)(5.60664)(5.59521)(5.592)(5.54981)(5.45843)(5.44853)(5.44098)(5.42837)(5.31394)(5.30016)(5.27455)(5.27174)(5.20512)(5.15919)(5.117)(5.09824)(5.07815)(5.01147)(4.97529)(4.97281)(4.96939)(4.9025)(4.87884)(4.86297)(4.86114)(4.79927)(4.78837)(4.77238)(4.7605)(4.71199)(4.70874)(4.69789)(4.66753)(4.62877)(4.62624)(4.62563)(4.57746)(4.53587)(4.53515)(4.52815)(4.46948)(4.44461)(4.42906)(4.4235)(4.38189)(4.37235)(4.35469)(4.35051)(4.31939)(4.3183)(4.30063)(4.29641)(4.26532)(4.26425)(4.23258)(4.2269)(4.19199)(4.18752)(4.17068)(4.16143)(4.13801)(4.13469)(4.12229)(4.11545)(4.10218)(4.09227)(4.08085)(4.06616)(4.0482)(4.04087)(4.03246)(4.02161)(4.01236)(4.00219)(3.99465)(3.98489)(3.9766)(3.97)(3.96245)(3.94911)(3.94083)(3.93722)(3.93309)(3.92579)(3.92322)(3.91822)(3.91452)(3.90797)(3.90567)(3.90067)(3.89873)(3.89216)(3.88812)(3.88312)(3.88118)(3.87461)(3.87057)(3.86695)(3.86282)(3.8555)(3.85295)(3.84796)(3.84601)(3.83768)(3.8354)(3.83559)(3.83566)(3.83591)(3.83606)(3.83107)(3.83088)(3.82255)(3.81851)(3.81864)(3.81871)(3.81903)(3.81918)(3.81595)(3.81575)(3.80565)(3.80163)(3.79802)(3.7978)(3.78654)(3.78401)(3.7842)(3.78421)(3.78459)(3.78468)(3.77809)(3.77411)(3.7571)(3.74891)(3.74903)(3.74911)(3.74942)(3.74958)(3.7497)(3.74971)(3.75002)(3.75024)(3.74701)(3.7468)(3.73846)(3.73269)(3.73122)(3.72925)(3.72091)(3.71514)(3.7135)(3.7113)(3.70199)(3.69752);
std::vector<double> WallLayerReynoldsStresses::Rvv=list_of<double>(0.0)(6.64783e-05)(0.0192787)(0.0252542)(0.0253244)(0.0253568)(0.0313377)(0.0373186)(0.0432941)(0.0463116)(0.0489944)(0.0491617)(0.049329)(0.05531)(0.0611613)(0.061469)(0.0644865)(0.0671423)(0.069771)(0.0701597)(0.0705915)(0.073609)(0.0761406)(0.0766264)(0.0796439)(0.0821161)(0.0826613)(0.0851336)(0.0856788)(0.0886962)(0.0905531)(0.0911145)(0.0917137)(0.094132)(0.0947311)(0.0971494)(0.0977486)(0.100766)(0.10313)(0.103778)(0.106796)(0.109111)(0.109813)(0.111341)(0.11283)(0.115087)(0.118811)(0.121068)(0.121829)(0.127049)(0.12781)(0.132123)(0.133791)(0.135993)(0.136808)(0.142789)(0.144932)(0.148765)(0.149709)(0.150039)(0.155492)(0.156698)(0.164998)(0.17201)(0.178872)(0.181516)(0.186738)(0.192053)(0.196923)(0.20565)(0.211816)(0.214356)(0.223832)(0.23662)(0.242163)(0.243459)(0.260861)(0.268709)(0.274802)(0.277396)(0.294707)(0.297956)(0.301012)(0.316271)(0.321515)(0.327298)(0.334984)(0.345035)(0.35518)(0.35776)(0.367223)(0.372079)(0.381116)(0.385709)(0.387466)(0.404189)(0.409174)(0.412159)(0.415767)(0.418874)(0.427987)(0.434012)(0.436957)(0.439706)(0.452499)(0.465082)(0.470303)(0.473113)(0.474687)(0.496882)(0.502177)(0.503818)(0.506979)(0.520927)(0.539206)(0.539938)(0.541416)(0.545886)(0.556913)(0.557185)(0.558085)(0.560303)(0.570374)(0.575779)(0.576965)(0.577335)(0.582184)(0.59445)(0.595779)(0.596693)(0.598003)(0.609883)(0.611554)(0.614658)(0.614999)(0.623075)(0.628529)(0.633538)(0.633617)(0.633701)(0.634026)(0.634203)(0.634547)(0.635021)(0.646165)(0.650105)(0.653083)(0.653464)(0.666312)(0.668577)(0.671896)(0.675557)(0.69051)(0.691119)(0.693154)(0.700285)(0.70939)(0.709411)(0.709416)(0.709734)(0.710009)(0.710014)(0.710055)(0.710399)(0.710545)(0.710674)(0.71072)(0.711064)(0.711143)(0.711338)(0.711384)(0.732682)(0.733431)(0.745521)(0.748413)(0.748757)(0.748769)(0.749031)(0.749078)(0.757278)(0.758839)(0.764725)(0.767957)(0.768246)(0.768301)(0.768508)(0.768622)(0.774914)(0.778377)(0.782369)(0.787502)(0.787723)(0.787845)(0.787986)(0.788167)(0.792551)(0.797918)(0.8019)(0.807047)(0.807201)(0.807323)(0.807464)(0.807711)(0.807865)(0.807988)(0.808128)(0.808376)(0.808463)(0.808653)(0.808793)(0.809041)(0.811778)(0.817724)(0.820043)(0.827854)(0.828008)(0.828197)(0.828271)(0.828519)(0.828672)(0.828795)(0.828935)(0.829184)(0.831642)(0.837013)(0.839107)(0.848064)(0.84815)(0.848339)(0.848413)(0.848662)(0.848815)(0.849004)(0.849011)(0.849327)(0.84948)(0.849602)(0.849676)(0.849992)(0.854327)(0.857804)(0.858013)(0.868872)(0.869024)(0.869146)(0.869154)(0.869536)(0.869622)(0.869811)(0.869819)(0.870201)(0.867673)(0.864071)(0.861888)(0.852584)(0.856906)(0.860382)(0.862487)(0.871464)(0.871616)(0.871739)(0.871746)(0.872062)(0.872281)(0.872403)(0.872411)(0.872727)(0.872946)(0.873001)(0.873076)(0.873392)(0.87361)(0.873666)(0.873741)(0.874057)(0.874209);
std::vector<double> WallLayerReynoldsStresses::Rww=list_of<double>(0.0)(0.00700923)(0.160793)(0.183347)(0.183672)(0.183822)(0.211489)(0.239157)(0.2668)(0.280759)(0.293169)(0.294053)(0.294937)(0.326532)(0.357443)(0.359068)(0.375008)(0.389038)(0.402925)(0.405318)(0.407977)(0.426558)(0.442147)(0.445139)(0.463719)(0.478943)(0.4823)(0.497524)(0.500881)(0.519462)(0.530896)(0.534844)(0.539057)(0.556064)(0.560277)(0.577284)(0.581497)(0.602717)(0.619344)(0.623899)(0.645119)(0.661404)(0.666339)(0.677082)(0.686256)(0.70015)(0.723085)(0.736979)(0.741665)(0.773808)(0.778494)(0.805052)(0.818726)(0.836781)(0.843463)(0.892494)(0.910062)(0.941481)(0.949225)(0.951172)(0.971931)(0.976522)(1.00812)(1.03481)(1.06093)(1.07266)(1.09583)(1.12199)(1.14595)(1.1889)(1.21547)(1.22642)(1.26725)(1.31694)(1.33541)(1.33973)(1.39774)(1.42676)(1.45248)(1.46343)(1.53651)(1.54852)(1.55871)(1.60958)(1.62706)(1.64634)(1.67196)(1.70911)(1.74662)(1.75616)(1.79114)(1.8091)(1.84251)(1.85949)(1.86598)(1.91187)(1.92555)(1.94008)(1.95765)(1.96978)(2.00534)(2.01859)(2.02507)(2.03111)(2.06622)(2.10075)(2.11507)(2.12279)(2.12539)(2.16211)(2.17087)(2.17331)(2.17803)(2.20569)(2.24194)(2.2451)(2.25149)(2.26458)(2.29687)(2.29766)(2.3003)(2.3068)(2.32874)(2.34051)(2.3431)(2.34383)(2.35243)(2.37419)(2.37655)(2.37835)(2.38092)(2.4068)(2.41044)(2.41721)(2.41795)(2.42532)(2.43029)(2.43486)(2.4369)(2.43954)(2.44977)(2.45533)(2.45571)(2.45618)(2.46733)(2.47127)(2.47424)(2.47459)(2.475)(2.47507)(2.47517)(2.47525)(2.47557)(2.4756)(2.47567)(2.47592)(2.48502)(2.48562)(2.48576)(2.4948)(2.49507)(2.49508)(2.49512)(2.49546)(2.49561)(2.49574)(2.49578)(2.49613)(2.49621)(2.4964)(2.49645)(2.49679)(2.49644)(2.49068)(2.4893)(2.47918)(2.47886)(2.47194)(2.4707)(2.46308)(2.46163)(2.46183)(2.46195)(2.46223)(2.46229)(2.45682)(2.45381)(2.44796)(2.44474)(2.44488)(2.44506)(2.44528)(2.4454)(2.44555)(2.44573)(2.44588)(2.44607)(2.43767)(2.42681)(2.41758)(2.41024)(2.40653)(2.39999)(2.39593)(2.39269)(2.39283)(2.39308)(2.39316)(2.39335)(2.38581)(2.37245)(2.36777)(2.35759)(2.35766)(2.35791)(2.35806)(2.35825)(2.35384)(2.33895)(2.32977)(2.32242)(2.31872)(2.31215)(2.30986)(2.30487)(2.30292)(2.29459)(2.29231)(2.28732)(2.28537)(2.2788)(2.27476)(2.26977)(2.26956)(2.26026)(2.25576)(2.25215)(2.2502)(2.24187)(2.23783)(2.2346)(2.23441)(2.22431)(2.22028)(2.21705)(2.21685)(2.20676)(2.20449)(2.1995)(2.19928)(2.18801)(2.18549)(2.18188)(2.17993)(2.17159)(2.16757)(2.16433)(2.16238)(2.15403)(2.15001)(2.14678)(2.14658)(2.13823)(2.13246)(2.12923)(2.129)(2.11969)(2.11326)(2.11162)(2.10965)(2.1013)(2.09554)(2.09407)(2.0921)(2.08375)(2.07974);

defineType(WallLayerReynoldsStresses);
addToFactoryTable(ReynoldsStressModel, WallLayerReynoldsStresses, Foam::dictionary);

class DNSVectorProfile
{
  interpolationTable<scalar> cmpt[3];
public:
  DNSVectorProfile
  (
    const std::string& databaseID, 
    const std::string& xname,
    const std::string& yname,
    const std::string& zname
  )
  {
    const char *names[] = {xname.c_str(), yname.c_str(), zname.c_str()};
    
    for (int i=0; i<3; i++)
    {
      arma::mat rvsyp=insight::refdatalib.getProfile(databaseID, names[i]);
      cmpt[i].resize(rvsyp.n_rows);
      cmpt[i].outOfBounds(interpolationTable<scalar>::CLAMP);
      for (size_t j=0; j<rvsyp.n_rows; j++)
      {
	cmpt[i].UList<Tuple2<scalar, scalar> >::operator[](j)=Tuple2<scalar, scalar>(rvsyp(j,0), rvsyp(j,1));
      }
    }
  }
  virtual ~DNSVectorProfile() {};
   
  virtual Foam::vector value(scalar yp) const
  {
    return Foam::vector(cmpt[0](yp), cmpt[1](yp), cmpt[2](yp));
  }
};

/**
 * return RMS profile from selected channel DNS
 */
class ChannelDNSReynoldsStresses
: public DNSVectorProfile,
  public ReynoldsStressModel
{
public:
  declareType("ChannelDNSReynoldsStresses");
  
  ChannelDNSReynoldsStresses(const Foam::dictionary&)
  : DNSVectorProfile("MKM_Channel", "590/Ruu_vs_yp", "590/Rvv_vs_yp", "590/Rww_vs_yp")
  {}
   
  virtual Foam::symmTensor operator()(const FlowProps& flow, Foam::scalar y) const
  {
    Foam::vector v=this->value(y*flow.Retau_) * Foam::sqr(flow.utau_);
    return Foam::symmTensor(v.x(), 0, 0, v.y(), 0, v.z());
  }
};

defineType(ChannelDNSReynoldsStresses);
addToFactoryTable(ReynoldsStressModel, ChannelDNSReynoldsStresses, Foam::dictionary);

/**
 * return RMS profile from selected Pipe DNS
 */
class PipeDNSReynoldsStresses
: public DNSVectorProfile,
  public ReynoldsStressModel
{
public:
  declareType("PipeDNSReynoldsStresses");
  
  PipeDNSReynoldsStresses(const Foam::dictionary&)
  : DNSVectorProfile("K_Pipe", "590/Rzz_vs_yp", "590/Rrr_vs_yp", "590/Rphiphi_vs_yp")
  {}
   
  virtual Foam::symmTensor operator()(const FlowProps& flow, scalar y) const
  {
    Foam::vector v=this->value(y*flow.Retau_) * Foam::sqr(flow.utau_);
    return Foam::symmTensor(v.x(), 0, 0, v.y(), 0, v.z());
  }
};

defineType(PipeDNSReynoldsStresses);
addToFactoryTable(ReynoldsStressModel, PipeDNSReynoldsStresses, Foam::dictionary);

/**
 * compute isotropic reynolds stresses from profile of TKE vs ydelta
 */
class TabulatedKReynoldsStresses
: public ReynoldsStressModel
{
  std::auto_ptr<Interpolator> ipol_;
  
public:
  declareType("TabulatedKReynoldsStresses");
  
  TabulatedKReynoldsStresses(const Foam::dictionary& dict)
  {
    loadData( fileName(dict.lookup("fileName")) );
  }
  
  TabulatedKReynoldsStresses(const boost::filesystem::path& fp)
  {
    loadData(fp);
  }
  
  void loadData(const boost::filesystem::path& fp)
  {
    arma::mat data;
    data.load(fp.c_str(), arma::arma_ascii);
    if (data.n_cols!=2)
    {
      insight::Warning
      (
	"Expected 2 columns, got "+lexical_cast<std::string>(data.n_cols)+"!\n"
	"Remaining columns are to be omitted!\n"
	"Check consistency of input!"
      );
    }
    ipol_.reset(new Interpolator(data));
  }
   
  virtual symmTensor operator()(const FlowProps& flow, scalar y) const
  {
    arma::mat k=ipol_->operator()(y/flow.delta_) * Foam::sqr(flow.utau_);
    double uPrimeSqr=k(0)/1.5;
    return symmTensor(uPrimeSqr, 0, 0, uPrimeSqr, 0, uPrimeSqr);
  }
};

defineType(TabulatedKReynoldsStresses);
addToFactoryTable(ReynoldsStressModel, TabulatedKReynoldsStresses, Foam::dictionary);

}

//=============================================================================
//=============================================================================
//=============================================================================

namespace Foam
{

/**
 * base class for bundling the algorithms to initialize an inflow generator patch
 */
class inflowInitializer
{
protected:
  dictionary dict_;
  word patchName_;
  
  boost::shared_ptr<insight::MeanVelocityModel> Umean_;
  boost::shared_ptr<insight::ReynoldsStressModel> RMS_;
  boost::shared_ptr<insight::LengthScaleModel> Ldelta_;
  
public:
  TypeName("inflowInitializer");
  
  declareRunTimeSelectionTable
        (
            autoPtr,
            inflowInitializer,
            istream,
            (
                Istream& is
            ),
            (is)
        );
	
  //- Selector
  static autoPtr<inflowInitializer > New
  (
      Istream& is
  )
  {
    word typeName(is);
    Info<< "Selecting initializer type " << typeName << endl;

    istreamConstructorTable::iterator cstrIter =
        istreamConstructorTablePtr_->find(typeName);

    if (cstrIter == istreamConstructorTablePtr_->end())
    {
        FatalErrorIn
        (
            "inflowInitializer::New()"
        )   << "Unknown turbulenceModel type " << typeName
            << endl << endl
            << "Valid inflowInitializer types are :" << endl
            << istreamConstructorTablePtr_->toc()
            << exit(FatalError);
    }

    return autoPtr<inflowInitializer>(cstrIter()(is));
  }

  inflowInitializer(Istream& is)
  : dict_(is),
    patchName_(dict_.lookup("patchName"))
  {
    std::string mvm_name=word(dict_.lookup("MeanVelocityModel"));
    std::string rms_name=word(dict_.lookup("ReynoldsStressModel"));
    std::string L_name=word(dict_.lookup("LengthScaleModel"));
    
    Umean_.reset
    (
      MeanVelocityModel::lookup
      (
	mvm_name,
	dict_.subDict(mvm_name+"Coeffs")
      )
    );
    RMS_.reset
    (
      ReynoldsStressModel::lookup
      (
	rms_name,
	dict_.subDict(rms_name+"Coeffs")
      )
    );
    Ldelta_.reset
    (
      LengthScaleModel::lookup
      (
	L_name,
	dict_.subDict(L_name+"Coeffs")
      )
    );
  }
  
  virtual ~inflowInitializer() 
  {}
  
  inflowGeneratorBaseFvPatchVectorField& inflowGeneratorPatchField(volVectorField& U) const
  {
    label patchI=U.mesh().boundaryMesh().findPatchID(patchName_);
    if (patchI<0)
    {
      FatalErrorIn("initialize")
      << "Patch "<<patchName_<<" does not exist!"
      << abort(FatalError);
    }
    
    return refCast<inflowGeneratorBaseFvPatchVectorField>(U.boundaryField()[patchI]);
  }
  
  virtual void initialize(volVectorField& U, scalar nu, bool checkStatistics=false) const =0;
  
  virtual autoPtr<inflowInitializer> clone() const =0;
};

defineTypeNameAndDebug(inflowInitializer, 0);
defineRunTimeSelectionTable(inflowInitializer, istream);


struct pipeFlowProps
: public FlowProps
{
//   Foam::vector Ubulk_;
//   double delta_;
//   double utau_;
//   double Retau_;
//   double Re_;

  pipeFlowProps(double D, const Foam::vector& Ubulk, double nu)
  {
   
    Ubulk_=Ubulk;
    delta_=0.5*D;
    Re_=mag(Ubulk_)*delta_/nu;
    Retau_=insight::PipeBase::Retau(Re_);
    utau_=mag(Ubulk_)*Retau_/Re_;
    
    Info
      <<"H="<<D
      <<", Ubulk="<<Ubulk_
      <<", Re="<<Re_
      <<", utau="<<utau_
      <<", Retau="<<Retau_
    <<endl;
  }
};

/**
 * initializer class for setting up an inlet with a fully developed pipe flow
 */
class pipeFlow
: public inflowInitializer
{
  scalar Ubulk_;
  
public:
  TypeName("pipeFlow");
  
  pipeFlow(Istream& is)
  : inflowInitializer(is),
    Ubulk_(readScalar(dict_.lookup("Ubulk")))
  {
  }
    
  virtual void initialize(volVectorField& U, scalar nu, bool checkStatistics=false) const
  {
    inflowGeneratorBaseFvPatchVectorField& ifpf = inflowGeneratorPatchField(U);
    const fvPatch& patch=ifpf.patch();
    
    point p0=gAverage(patch.Cf());
    vector axis=-gAverage(patch.Sf()/patch.magSf());
    axis/=mag(axis);
    vectorField rv=patch.Cf()-p0;
    rv-=axis*(rv&axis);
    
    scalar D = 2.0*max(mag(rv));
    
    pipeFlowProps flow(D, axis*Ubulk_, nu);
//     flow.delta_=0.5*D;
//     flow.Ubulk_=axis*Ubulk_;
//     flow.Re_=Ubulk_*0.5*D/nu;
//     flow.Retau_=insight::PipeBase::Retau(flow.Re_);
//     flow.utau_=Ubulk_*flow.Retau_/flow.Re_;
//     Info<<"D="<<D<<", center="<<p0<<", flow dir="<<axis<<", Re="<<flow.Re_<<", utau="<<flow.utau_<<", Retau="<<flow.Retau_<<endl;
    
    forAll(patch.Cf(), fi)
    {
      scalar r=mag(rv[fi]);
      scalar y=(0.5*D-r);
      
      ifpf.Umean()[fi]= (*Umean_)(flow, y);
      
      vector e_radial(rv[fi]/mag(rv[fi]));
      vector e_tan=axis^e_radial;
      
      tensor ev(axis, e_radial, e_tan); // eigenvectors => rows      
      tensor L = ev.T() & (*Ldelta_)(flow, y) & ev;

      ifpf.L()[fi] = symmTensor(L.xx(), L.xy(), L.xz(),
					L.yy(), L.yz(),
						L.zz());
      
      tensor R=ev.T() & (*RMS_)(flow, y) & ev;
      ifpf.R()[fi] = symmTensor(R.xx(), R.xy(), R.xz(),
					R.yy(), R.yz(),
						R.zz());
    }
    
    if (checkStatistics)
      ifpf.computeConditioningFactor();
  }

  virtual autoPtr<inflowInitializer> clone() const
  {
    return autoPtr<inflowInitializer>(new pipeFlow(/*patchName_, Ubulk_, D_, p0_, axis_*/*this));
  }

};

defineTypeNameAndDebug(pipeFlow, 0);
addToRunTimeSelectionTable(inflowInitializer, pipeFlow, istream);
   
struct channelFlowProps
: public FlowProps
{
//   Foam::vector Ubulk_;
//   double delta_;
//   double utau_;
//   double Retau_;
//   double Re_;

  channelFlowProps(double H, const Foam::vector& Ubulk, double nu)
  {
    Ubulk_=Ubulk;
    delta_=0.5*H;
    Re_=mag(Ubulk_)*delta_/nu;
    Retau_=insight::ChannelBase::Retau(Re_);
    utau_=mag(Ubulk_)*Retau_/Re_;
    
    Info
      <<"H="<<H
      <<", Ubulk="<<Ubulk_
      <<", Re="<<Re_
      <<", utau="<<utau_
      <<", Retau="<<Retau_
    <<endl;
  }
};

class channelFlow
: public inflowInitializer
{
  scalar Ubulk_;
  vector vertical_;
  
public:
  TypeName("channelFlow");
  

  channelFlow(Istream& is)
  : inflowInitializer(is),
    Ubulk_(readScalar(dict_.lookup("Ubulk"))),
    vertical_(dict_.lookupOrDefault<vector>("vertical", vector(0,1,0)))
  {
  }
    
  virtual void initialize(volVectorField& U, scalar nu, bool checkStatistics=false) const
  {
    inflowGeneratorBaseFvPatchVectorField& ifpf = inflowGeneratorPatchField(U);
    const fvPatch& patch=ifpf.patch();
    
    point p0=gAverage(patch.Cf());
    vector e_ax=-gAverage(patch.Sf()/patch.magSf());
    e_ax/=mag(e_ax);
    vector e_v=vertical_/mag(vertical_);
    vector e_span=e_ax^e_v; 
    e_span/=mag(e_span);
    vectorField hv=patch.Cf()-p0; 
    hv-=e_ax*(hv&e_ax);
    hv-=e_span*(hv&e_span);
    scalar H=2.*max(mag(hv));
    scalar delta=0.5*H;
    
    channelFlowProps flow(H, Ubulk_*e_ax, nu);
    
    tensor ev(e_ax, e_v, e_span); // eigenvectors => rows
    
    forAll(patch.Cf(), fi)
    {
      scalar h=mag(hv[fi]);
      scalar y=((0.5*H)-h);
      
      ifpf.Umean()[fi]=(*Umean_)(flow, y);
      
      
      tensor L = ev.T() & (*Ldelta_)(flow, y) & ev;

      ifpf.L()[fi] = symmTensor(L.xx(), L.xy(), L.xz(),
					L.yy(), L.yz(),
						L.zz());
      
      tensor R=ev.T() & (*RMS_)(flow, y) & ev;
      ifpf.R()[fi] = symmTensor(R.xx(), R.xy(), R.xz(),
					R.yy(), R.yz(),
						R.zz());
    }
    
    if (checkStatistics)
      ifpf.computeConditioningFactor();
  }

  virtual autoPtr<inflowInitializer> clone() const
  {
    return autoPtr<inflowInitializer>(new channelFlow(/*patchName_, Ubulk_, D_, p0_, axis_*/*this));
  }

};

defineTypeNameAndDebug(channelFlow, 0);
addToRunTimeSelectionTable(inflowInitializer, channelFlow, istream);

}

int main(int argc, char *argv[])
{
#ifdef OF16ext
  argList::validOptions.insert("checkStatistics", "");
#else
  argList::addBoolOption("checkStatistics", "do a precursor run of the boundary field generation alone and collect statistics");
#endif
  
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"
  
  bool checkStatistics = args.optionFound("checkStatistics");
  
  wallDist y(mesh);
  
  Info<< "Reading combustion properties\n" << endl;

  IOdictionary inflowProperties
  (
      IOobject
      (
	  "inflowProperties",
	  runTime.constant(),
	  mesh,
	  IOobject::MUST_READ,
	  IOobject::NO_WRITE
      )
  );

  PtrList<inflowInitializer> inits(inflowProperties.lookup("initializers"));

  IOdictionary transportProperties
  (
      IOobject
      (
	  "transportProperties",
	  runTime.constant(),
	  mesh,
	  IOobject::MUST_READ,
	  IOobject::NO_WRITE
      )
  );

  dimensionedScalar nu
  (
      transportProperties.lookup("nu")
  );
    
  Info << "Reading field U\n" << endl;
  volVectorField U
  (
      IOobject
      (
	  "U",
	  runTime.timeName(),
	  mesh,
	  IOobject::MUST_READ,
	  IOobject::AUTO_WRITE
      ),
      mesh
  );

  forAll(inits, i)
    inits[i].initialize(U, nu.value(), checkStatistics);
  
  U.write();

  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
