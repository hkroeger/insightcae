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


class VelocityInletBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (velocity, arma::mat, vec3(0,0,0))
    (T, double, 300.0)
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
  virtual void setField_p(OFDictData::dict& BC) const;
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
  typedef arma::mat CoeffList;
  typedef boost::variant<double, CoeffList> LengthScaleProfile;
  
  struct inflowInitializer
  {
    typedef boost::shared_ptr<inflowInitializer> Ptr;
    
    virtual ~inflowInitializer();
    virtual std::string type() const =0;
    virtual void addToInitializerList
    (
      OFDictData::dict& initdict, 
      const std::string& patchName,
      const arma::mat& Ubulk,
      const ParameterSet& params
    ) const;
    static ParameterSet defaultParameters();
  };
  
//   struct isotropicTurbulenceInflowInitializer
//   {
//     double Ubulk;
//     arma::mat RMS;
//     
//     virtual void addToInitializerList(OFDictData::list&) const;
//   };
  
  struct pipeInflowInitializer
  : public inflowInitializer
  {
    pipeInflowInitializer();
    virtual std::string type() const;
    virtual void addToInitializerList
    (
      OFDictData::dict& initdict, 
      const std::string& patchName, 
      const arma::mat& Ubulk,
      const ParameterSet& params
    ) const;
  };

  struct channelInflowInitializer
  : public inflowInitializer
  {
    channelInflowInitializer();
    virtual std::string type() const;
    virtual void addToInitializerList
    (
      OFDictData::dict& initdict, 
      const std::string& patchName, 
      const arma::mat& Ubulk,
      const ParameterSet& params
    ) const;
  };

  CPPX_DEFINE_OPTIONCLASS(Parameters, VelocityInletBC::Parameters,
    (type, std::string, "inflowGenerator<hatSpot>")
    (uniformConvection, bool, false)
    (initializer, inflowInitializer::Ptr, inflowInitializer::Ptr())
    (delta, double, 1.0)
    (volexcess, double, 16.0)
    (longLengthScale, LengthScaleProfile, 1.0)
    (latLengthScale, LengthScaleProfile, 1.0)
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
  virtual void initInflowBC(const boost::filesystem::path& location, const ParameterSet& iniparams) const;

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
