/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INSIGHT_BASICCASEELEMENTS_H
#define INSIGHT_BASICCASEELEMENTS_H

#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{


class OpenFOAMCase;
class OFdicts;
  
/**
 Manages basic settings in controlDict, fvSchemes, fvSolution, list of fields
 */
class FVNumerics
: public OpenFOAMCaseElement
{
  
public:
  FVNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



/**
 Manages basic settings in faSchemes, faSolution
 */
class FaNumerics
: public OpenFOAMCaseElement
{
  
public:
  FaNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



/**
 Manages basic settings in tetFemSolution
 */
class tetFemNumerics
: public OpenFOAMCaseElement
{
  
public:
  tetFemNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



OFDictData::dict stdAsymmSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict stdSymmSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict smoothSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict GAMGSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict GAMGPCGSolverSetup(double tol=1e-7, double reltol=0.0);


class MeshingNumerics
: public FVNumerics
{
public:
  MeshingNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class simpleFoamNumerics
: public FVNumerics
{
public:
  simpleFoamNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class pimpleFoamNumerics
: public FVNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (deltaT, double, 1.0)
    (adjustTimeStep, bool, true)
    (maxCo, double, 0.45)
    (maxDeltaT, double, 1.0)
    (LES, bool, false)
  )

protected:
  Parameters p_;

public:
  pimpleFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class simpleDyMFoamNumerics
: public simpleFoamNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      (FEMinterval, int, 10)
  )

protected:
  Parameters p_;

public:
  simpleDyMFoamNumerics(OpenFOAMCase& c, Parameters const& params = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class cavitatingFoamNumerics
: public FVNumerics
{
public:
  cavitatingFoamNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class interFoamNumerics
: public FVNumerics
{
protected:
  std::string pname_;
public:
  interFoamNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  inline const std::string& pressureFieldName() const { return pname_; }
};

class LTSInterFoamNumerics
: public interFoamNumerics
{
public:
  LTSInterFoamNumerics(OpenFOAMCase& c);
  //virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
};

class interPhaseChangeFoamNumerics
: public interFoamNumerics
{
public:
  interPhaseChangeFoamNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class FSIDisplacementExtrapolationNumerics
: public FaNumerics
{
public:
  FSIDisplacementExtrapolationNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class thermodynamicModel
: public OpenFOAMCaseElement
{
public:
  thermodynamicModel(OpenFOAMCase& c);
};



class cavitatingFoamThermodynamics
: public thermodynamicModel
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (psiv, double, 2.5e-6)
    (psil, double, 5e-7)
    (pSat, double, 2000.0)
    (rholSat, double, 1025.0)
    (rhoMin, double, 0.001)
  )

protected:
  Parameters p_;
  
public:
  cavitatingFoamThermodynamics(OpenFOAMCase& c, Parameters const& p = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class gravity
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (g, arma::mat, vec3(0, 0, -9.81))
  )

protected:
  Parameters p_;
  
public:
  gravity(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class transportModel
: public OpenFOAMCaseElement
{
public:
  transportModel(OpenFOAMCase& c);
};


class MRFZone
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (name, std::string, "rotor")
    (rpm, double, 1000.0)
    (nonRotatingPatches, std::vector<std::string>, std::vector<std::string>())
    (rotationCentre, arma::mat, vec3(0,0,0))
    (rotationAxis, arma::mat, vec3(0,0,1))
  )

protected:
  Parameters p_;

public:
  MRFZone(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class PressureGradientSource
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (Ubar, arma::mat, vec3(0,0,0))
  )

protected:
  Parameters p_;

public:
  PressureGradientSource(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class singlePhaseTransportProperties
: public transportModel
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (nu, double, 1e-6)
  )

protected:
  Parameters p_;

public:
  singlePhaseTransportProperties(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class twoPhaseTransportProperties
: public transportModel
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (nu1, double, 1e-6)
    (rho1, double, 1025.0)
    (nu2, double, 1.5e-5)
    (rho2, double, 1.0)
    (sigma, double, 0.07)
  )

protected:
  Parameters p_;

public:
  twoPhaseTransportProperties(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



namespace phaseChangeModels
{
  
  
  
class phaseChangeModel
{
public:
  virtual void addIntoDictionaries(OFdicts& dictionaries) const =0;
};

typedef boost::shared_ptr<phaseChangeModel> Ptr;

class SchnerrSauer: public phaseChangeModel
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (n, double, 1.6e13)
    (dNuc, double, 2.0e-6)
    (Cc, double, 1.0)
    (Cv, double, 1.0)
  )

protected:
  Parameters p_;

public:
  SchnerrSauer(Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



}



class cavitationTwoPhaseTransportProperties
: public twoPhaseTransportProperties
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, twoPhaseTransportProperties::Parameters,
    (model, phaseChangeModels::Ptr, 
      phaseChangeModels::Ptr( new phaseChangeModels::SchnerrSauer() ))
    (psat, double, 2300.0)
  )

protected:
  Parameters p_;

public:
  cavitationTwoPhaseTransportProperties(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class dynamicMesh
: public OpenFOAMCaseElement
{
public:
  dynamicMesh(OpenFOAMCase& c);
};



class velocityTetFEMMotionSolver
: public dynamicMesh
{
  tetFemNumerics tetFemNumerics_;
public:
  velocityTetFEMMotionSolver(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class displacementFvMotionSolver
: public dynamicMesh
{
public:
  displacementFvMotionSolver(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class turbulenceModel
: public OpenFOAMCaseElement
{
public:
  typedef boost::tuple<OpenFOAMCase&> ConstrP;
  declareFactoryTable(turbulenceModel, ConstrP);  

public:
  declareType("turbulenceModel");

  turbulenceModel(OpenFOAMCase& c);
  turbulenceModel(const ConstrP& c);
  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const =0;
};

class RASModel
: public turbulenceModel
{

public:
  declareType("RASModel");

  RASModel(OpenFOAMCase& c);
  RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  

};

class LESModel
: public turbulenceModel
{

public:
  declareType("LESModel");

  LESModel(OpenFOAMCase& c);
  LESModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  

};

class laminar_RASModel
: public RASModel
{
public:
  declareType("laminar");

  laminar_RASModel(OpenFOAMCase& c);
  laminar_RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class oneEqEddy_LESModel
: public LESModel
{
protected:
  void addFields();
  
public:
  declareType("oneEqEddy");
  
  oneEqEddy_LESModel(OpenFOAMCase& c);
  oneEqEddy_LESModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class dynSmagorinsky_LESModel
: public LESModel
{
protected:
  void addFields();
  
public:
  declareType("dynSmagorinsky");
  
  dynSmagorinsky_LESModel(OpenFOAMCase& c);
  dynSmagorinsky_LESModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class kOmegaSST_RASModel
: public RASModel
{
protected:
  void addFields();
  
public:
  declareType("kOmegaSST");
  
  kOmegaSST_RASModel(OpenFOAMCase& c);
  kOmegaSST_RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class LEMOSHybrid_RASModel
: public kOmegaSST_RASModel
{
protected:
  void addFields();
  
public:
  declareType("LEMOSHybrid");
  
  LEMOSHybrid_RASModel(OpenFOAMCase& c);
  LEMOSHybrid_RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class kOmegaSST_LowRe_RASModel
: public kOmegaSST_RASModel
{
public:
  declareType("kOmegaSST_LowRe");
  
  kOmegaSST_LowRe_RASModel(OpenFOAMCase& c);
  kOmegaSST_LowRe_RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class kOmegaSST2_RASModel
: public kOmegaSST_RASModel
{
protected:
  void addFields();

public:
  declareType("kOmegaSST2");
  
  kOmegaSST2_RASModel(OpenFOAMCase& c);
  kOmegaSST2_RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};



/*
 * Manages the configuration of a single patch, i.e. one BoundaryCondition-object 
 * needs to know proper BC's for all fields on the given patch
 */
class BoundaryCondition
: public OpenFOAMCaseElement
{
protected:
  std::string patchName_;
  std::string type_;
  int nFaces_;
  int startFace_;
  
public:
  BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict);
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const =0;
  virtual void addOptionsToBoundaryDict(OFDictData::dict& bndDict) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  static void insertIntoBoundaryDict
  (
    OFdicts& dictionaries, 
    const std::string& patchName,
    const OFDictData::dict& bndsubd
  );

  inline const std::string patchName() const { return patchName_; }
  inline const std::string type() const { return type_; }
  
  virtual bool providesBCsForPatch(const std::string& patchName) const;
};



class SimpleBC
: public BoundaryCondition
{
protected:
  std::string className_;
  
public:
  SimpleBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className);
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};

class CyclicPairBC
: public OpenFOAMCaseElement
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (prefixName, std::string, "")
//   )
//   
// protected:
//   Parameters p_;
//   
protected:
  std::string patchName_;
  int nFaces_, nFaces1_;
  int startFace_, startFace1_;

public:
  CyclicPairBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;

  virtual bool providesBCsForPatch(const std::string& patchName) const;
};

class GGIBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (shadowPatch, std::string, "")
    (separationOffset, arma::mat, vec3(0,0,0))
    (bridgeOverlap, bool, true)
    (zone, std::string, "")
  )
  
protected:
  Parameters p_;
  
public:
  GGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p = Parameters() );
  virtual void addOptionsToBoundaryDict(OFDictData::dict& bndDict) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};

class CyclicGGIBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (shadowPatch, std::string, "")
    (separationOffset, arma::mat, vec3(0,0,0))
    (bridgeOverlap, bool, true)
    (rotationCentre, arma::mat, vec3(0,0,0))
    (rotationAxis, arma::mat, vec3(0,0,1))
    (rotationAngle, double, 0.0)
    (zone, std::string, "")
  )
  
protected:
  Parameters p_;
  
public:
  CyclicGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p = Parameters() );
  virtual void addOptionsToBoundaryDict(OFDictData::dict& bndDict) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};


namespace multiphaseBC
{
  
class multiphaseBC
{
public:
  // return true, if this field was handled, false otherwise
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const =0;
};

typedef boost::shared_ptr<multiphaseBC> Ptr;

class uniformPhases : public multiphaseBC
{
public:
  typedef std::map<std::string, double> PhaseFractionList;
  
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (phasefractions, PhaseFractionList, PhaseFractionList())
  )

protected:
  Parameters p_;

public:
  uniformPhases( Parameters const& p = Parameters() );
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

}


class SuctionInletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (pressure, double, 0.0)
    (rho, double, 1025.0)
    (gamma, double, 1.0)
    (phiName, std::string, "phi")
    (psiName, std::string, "none")
    (rhoName, std::string, "none")
    (UName, std::string, "U")
    (phasefractions, multiphaseBC::Ptr, multiphaseBC::Ptr( new multiphaseBC::uniformPhases() ))
  )
  
protected:
  Parameters p_;
  
public:
  SuctionInletBC
  (
    OpenFOAMCase& c, 
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};


class VelocityInletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (velocity, arma::mat, vec3(0,0,0))
    (rho, double, 1025.0)
    (turbulenceIntensity, double, 0.05)
    (mixingLength, double, 1.0)
    (phasefractions, multiphaseBC::Ptr, multiphaseBC::Ptr( new multiphaseBC::uniformPhases() ))
  )
  
protected:
  Parameters p_;
  
public:
  VelocityInletBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void setField_U(OFDictData::dict& BC) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};


class TurbulentVelocityInletBC
: public VelocityInletBC
{

public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, VelocityInletBC::Parameters,
    (structureType, std::string, "hatSpot")
  )
  
protected:
  Parameters p_;

public:
  TurbulentVelocityInletBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void setField_U(OFDictData::dict& BC) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
  virtual void initInflowBC(const boost::filesystem::path& location) const;
};


class PressureOutletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (pressure, double, 0.0)
    (rho, double, 1025.0)
  )
  
protected:
  Parameters p_;
  
public:
  PressureOutletBC
  (
    OpenFOAMCase& c, 
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};



class MeshMotionBC
{
public:
  MeshMotionBC();
  virtual ~MeshMotionBC();
  
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const =0;
  virtual MeshMotionBC* clone() const =0;
};



class NoMeshMotion
: public MeshMotionBC
{
public:
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
  virtual MeshMotionBC* clone() const;
};



extern NoMeshMotion noMeshMotion;



class CAFSIBC
: public MeshMotionBC
{
public:
  typedef std::map<double, double> RelaxProfile;
  
  typedef boost::variant<double, RelaxProfile> RelaxParameter;
  
  CPPX_DEFINE_OPTIONCLASS( Parameters, CPPX_OPTIONS_NO_BASE,
	(FEMScratchDir, boost::filesystem::path, "")
	(clipPressure, double, -100.0)
        (relax, RelaxParameter, RelaxParameter(0.2) )
        (pressureScale, double, 1e-3)
        (oldPressure, boost::shared_ptr<double>, boost::shared_ptr<double>() )
  )
  
protected:
  Parameters p_;
  
public:
  CAFSIBC(const Parameters& p);

  virtual ~CAFSIBC();

  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
  virtual MeshMotionBC* clone() const;
};



class WallBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (wallVelocity, arma::mat, vec3(0., 0., 0.))
    (meshmotion, boost::shared_ptr<MeshMotionBC>, boost::shared_ptr<MeshMotionBC>(noMeshMotion.clone()) )
  )
  
protected:
  Parameters p_;
  
public:
  WallBC
  (
    OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
    Parameters const &p = Parameters()
  );
  
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
  virtual void addOptionsToBoundaryDict(OFDictData::dict& bndDict) const;
};

}

#endif // INSIGHT_BASICCASEELEMENTS_H
