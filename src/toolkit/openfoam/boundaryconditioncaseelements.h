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
 *
 */


#ifndef INSIGHT_BOUNDARYCONDITIONCASEELEMENTS_H
#define INSIGHT_BOUNDARYCONDITIONCASEELEMENTS_H

#include "openfoam/numericscaseelements.h"
#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{




/**
 * Interface which wraps different types of prescribing field data on boundaries.
 * Works together with OpenFOAM class "FieldDataProvider".
 */
class FieldData
{
public:
  
protected:
  boost::shared_ptr<SelectableSubsetParameter> p_;
  
public:
  /**
   * sets all parameters for the most simple type of field data description (uniform, steady scalar value)
   */
  FieldData(double uniformSteadyValue);
  
  /**
   * sets all parameters for the most simple type of field data description (uniform, steady value)
   */
  FieldData(const arma::mat& uniformSteadyValue);
  
  /**
   * takes config from a parameterset
   */
  FieldData(const SelectableSubsetParameter& p);
  
  /**
   * returns according dictionary entry for OF
   */
  OFDictData::data sourceEntry() const;
  
  void setDirichletBC(OFDictData::dict& BC) const;
  
  /**
   * return some representative value of the prescribed data. 
   * Required e.g. for deriving turbulence qtys when velocity distributions are prescribed.
   */
  arma::mat representativeValue() const;
  
  /**
   * returns a proper parameterset for this entity
   * @reasonable_value: the number of components determines the rank of the field
   */
  static Parameter* defaultParameter(const arma::mat& reasonable_value, const std::string& description="Origin of the prescribed value");
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
  virtual ~multiphaseBC();
  
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  // return true, if this field was handled, false otherwise
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const =0;
};

typedef boost::shared_ptr<multiphaseBC> Ptr;

class uniformPhases : public multiphaseBC
{
public:
  typedef std::map<std::string, double> PhaseFractionList;

protected:
  PhaseFractionList phaseFractions_;

public:
  uniformPhases();
  uniformPhases( const uniformPhases& o);
  uniformPhases( const PhaseFractionList& p0 );
  uniformPhases& set(const std::string& name, double val);
  inline Ptr toPtr() { return Ptr(new uniformPhases(*this)); }
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
    (T, double, 300.0)
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




typedef boost::fusion::tuple<double, double> uniformIntensityAndLengthScale;

enum oneEqnValueType { nuTilda, RS };
typedef boost::fusion::tuple<oneEqnValueType, FieldData> oneEqn;

enum twoEqnValueType { kEpsilon, kOmega, REpsilon, RSL };
typedef boost::fusion::tuple<twoEqnValueType, FieldData, FieldData> twoEqn;




class TurbulenceSpecification
: public boost::variant<
    uniformIntensityAndLengthScale,
    oneEqn,
    twoEqn
    >
{
public:
    TurbulenceSpecification(const uniformIntensityAndLengthScale& uil);
    TurbulenceSpecification(const oneEqn&);
    TurbulenceSpecification(const twoEqn&);
    
    void setDirichletBC_k(OFDictData::dict& BC, double U) const;
    void setDirichletBC_omega(OFDictData::dict& BC, double U) const;
    void setDirichletBC_epsilon(OFDictData::dict& BC, double U) const;
    void setDirichletBC_nuTilda(OFDictData::dict& BC, double U) const;
    void setDirichletBC_R(OFDictData::dict& BC, double U) const;
};




class VelocityInletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (velocity, FieldData, FieldData(vec3(0,0,0)) )
    (T, FieldData, FieldData(300.0) )
    (rho, FieldData, FieldData(1025.0) )
    (turbulence, TurbulenceSpecification, TurbulenceSpecification(uniformIntensityAndLengthScale(0.01, 1e-3)) )
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
  virtual void setField_p(OFDictData::dict& BC) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};




class ExptDataInletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (points, arma::mat, vec3(0,0,0))
    (velocity, arma::mat, vec3(0,0,0))
    (TKE, arma::mat, arma::ones(1)*1e-3)
    (epsilon, arma::mat, arma::ones(1)*1e-3)
    (phasefractions, multiphaseBC::Ptr, multiphaseBC::Ptr( new multiphaseBC::uniformPhases() ))
  )
  
protected:
  Parameters p_;
  
public:
  ExptDataInletBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void addDataDict(OFdicts& dictionaries, const std::string& prefix, const std::string& fieldname, const arma::mat& data) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};




class CompressibleInletBC
: public VelocityInletBC
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, VelocityInletBC::Parameters,
    (pressure, double, 1e5)
  )
  
protected:
  Parameters p_;
  
public:
  CompressibleInletBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void setField_p(OFDictData::dict& BC) const;
};




class TurbulentVelocityInletBC
: public VelocityInletBC
{

public:
//   typedef arma::mat CoeffList;
//   typedef boost::variant<double, CoeffList> LengthScaleProfile;
//   
//   
//   struct inflowInitializer
//   {
//     typedef boost::shared_ptr<inflowInitializer> Ptr;
//     
//     virtual ~inflowInitializer();
//     virtual std::string type() const =0;
//     virtual void addToInitializerList
//     (
//       OFDictData::dict& initdict, 
//       const std::string& patchName,
//       const arma::mat& Ubulk,
//       const ParameterSet& params
//     ) const;
//     static ParameterSet defaultParameters();
//   };
//   
// 
//   
//   struct pipeInflowInitializer
//   : public inflowInitializer
//   {
//     pipeInflowInitializer();
//     virtual std::string type() const;
//     virtual void addToInitializerList
//     (
//       OFDictData::dict& initdict, 
//       const std::string& patchName, 
//       const arma::mat& Ubulk,
//       const ParameterSet& params
//     ) const;
//   };
// 
//   
//   struct channelInflowInitializer
//   : public inflowInitializer
//   {
//     channelInflowInitializer();
//     virtual std::string type() const;
//     virtual void addToInitializerList
//     (
//       OFDictData::dict& initdict, 
//       const std::string& patchName, 
//       const arma::mat& Ubulk,
//       const ParameterSet& params
//     ) const;
//   };

//   CPPX_DEFINE_OPTIONCLASS(Parameters, VelocityInletBC::Parameters,
//     (type, std::string, "inflowGenerator<hatSpot>")
//     (uniformConvection, bool, false)
//     (turbulence, boost::shared_ptr<SubsetParameter>, boost::shared_ptr<SubsetParameter>(defaultParameters()) )
//     (initializer, inflowInitializer::Ptr, inflowInitializer::Ptr())
//     (delta, double, 1.0)
//     (volexcess, FieldData, FieldData(16.0) )
//     (lengthScale, FieldData, FieldData(1.0) )
//   )
 

//   DEFINE_SubsetParameter	
//   (
//       DEFINE_ParameterSet
//       (
// 	("uniformConvection", new BoolParameter(false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
// 	("volexcess", new DoubleParameter(16.0, "Volumetric overlapping of spots"))
// 	(
// 	  "type", new SelectionParameter(0, 
// 	    list_of<string>
// 	    ("inflowGenerator<hatSpot>")
// 	    ("inflowGenerator<gaussianSpot>")
// 	    ("inflowGenerator<decayingTurbulenceSpot>")
// 	    ("inflowGenerator<decayingTurbulenceVorton>")
// 	    ("inflowGenerator<anisotropicVorton>")
// 	    ("modalTurbulence")
// 	  , 
// 	  "Type of inflow generator")
// 	)
// 	("L", FieldData::defaultParameter(vec3(1,1,1), "Origin of the prescribed integral length scale field"))
// 	("R", FieldData::defaultParameter(arma::zeros(6), "Origin of the prescribed reynolds stress field"))
// 	.convert_to_container<ParameterSet::EntryList>()
//       ), 
//       "Inflow generator parameters"
//   )

  ISP_DEFINE_PARAMETERSET
  (
    Parameters,
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, bool, uniformConvection, BoolParameter, (false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, double, volexcess, DoubleParameter, (2.0, "Volumetric overlapping of spots"))
    ISP_DEFINE_SIMPLEPARAMETER
    (
      Parameters, int, type, 
      SelectionParameter, 
      (
	0, 
	boost::assign::list_of<std::string>
	("inflowGenerator<hatSpot>")
	("inflowGenerator<gaussianSpot>")
	("inflowGenerator<decayingTurbulenceSpot>")
	("inflowGenerator<decayingTurbulenceVorton>")
	("inflowGenerator<anisotropicVorton>")
	("modalTurbulence"), 
	"Type of inflow generator"
      )
    )
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, boost::filesystem::path, R, PathParameter, ("", "Path to profile with Reynolds Stresses"))
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, boost::filesystem::path, L, PathParameter, ("", "Path to profile with integral length scale"))
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, boost::filesystem::path, umean, PathParameter, ("", "Path to profile with mean velocity"))
  )
  
protected:
  ParameterSet p_;

public:
  TurbulentVelocityInletBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    ParameterSet const& p = Parameters::makeWithDefaults()
  );
  
  virtual void setField_U(OFDictData::dict& BC) const;
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
//   virtual void initInflowBC(const boost::filesystem::path& location, const ParameterSet& iniparams) const;

//   inline static ParameterSet defaultParameters()
//    { return Parameters::makeWithDefaults(); }
  static ParameterSet defaultParameters();
};


class PressureOutletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (pressure, double, 0.0)
    (fixMeanValue, bool, false)
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


class PotentialFreeSurfaceBC
: public BoundaryCondition
{
public:
  PotentialFreeSurfaceBC
  (
    OpenFOAMCase& c, 
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict
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
    (wallVelocity, arma::mat, vec3(0., 0., 0.)) // interpreted as omega vector, if "rotating" is set to true
    (rotating, bool, false)
    (CofR, arma::mat, vec3(0,0,0))
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
