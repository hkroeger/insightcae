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


#ifndef INSIGHT_BASICCASEELEMENTS_H
#define INSIGHT_BASICCASEELEMENTS_H

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
  virtual ~phaseChangeModel();
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
  
  enum AccuracyRequirement { AC_RANS, AC_LES, AC_DNS };

public:
  declareType("turbulenceModel");

  turbulenceModel(OpenFOAMCase& c);
  turbulenceModel(const ConstrP& c);
  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const =0;
  
  virtual AccuracyRequirement minAccuracyRequirement() const =0;
};

class RASModel
: public turbulenceModel
{

public:
  declareType("RASModel");

  RASModel(OpenFOAMCase& c);
  RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual AccuracyRequirement minAccuracyRequirement() const;
};

class LESModel
: public turbulenceModel
{

public:
  declareType("LESModel");

  LESModel(OpenFOAMCase& c);
  LESModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual AccuracyRequirement minAccuracyRequirement() const;
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

class dynOneEqEddy_LESModel
: public LESModel
{
protected:
  void addFields();
  
public:
  declareType("dynOneEqEddy");
  
  dynOneEqEddy_LESModel(OpenFOAMCase& c);
  dynOneEqEddy_LESModel(const ConstrP& c);
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

class kEpsilon_RASModel
: public RASModel
{
protected:
  void addFields();
  
public:
  declareType("kEpsilon");
  
  kEpsilon_RASModel(OpenFOAMCase& c);
  kEpsilon_RASModel(const ConstrP& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const;
};

class SpalartAllmaras_RASModel
: public RASModel
{
protected:
  void addFields();
  
public:
  declareType("SpalartAllmaras");
  
  SpalartAllmaras_RASModel(OpenFOAMCase& c);
  SpalartAllmaras_RASModel(const ConstrP& c);
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

}

#endif // INSIGHT_BASICCASEELEMENTS_H
