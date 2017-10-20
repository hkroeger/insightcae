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


#ifndef INSIGHT_TURBULENCEMODELCASEELEMENTS_H
#define INSIGHT_TURBULENCEMODELCASEELEMENTS_H

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



class turbulenceModel
: public OpenFOAMCaseElement
{
    
public:

  declareFactoryTable(turbulenceModel, LIST(OpenFOAMCase& ofc), LIST(ofc));  
  
  enum AccuracyRequirement { AC_RANS, AC_LES, AC_DNS };

public:
  declareType("turbulenceModel");

  turbulenceModel(OpenFOAMCase& c);
  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const =0;
  virtual OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const =0; 
  
  virtual AccuracyRequirement minAccuracyRequirement() const =0;

  static std::string category() { return "Turbulence Model"; }
};




class RASModel
: public turbulenceModel
{

public:
  declareType("RASModel");

  RASModel(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const; 
  virtual AccuracyRequirement minAccuracyRequirement() const;
};

class LESModel
: public turbulenceModel
{

public:
  declareType("LESModel");

  LESModel(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual OFDictData::dict& modelPropsDict(OFdicts& dictionaries) const; 
  virtual AccuracyRequirement minAccuracyRequirement() const;
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
};

class laminar_RASModel
: public RASModel
{
public:
  declareType("laminar");

  laminar_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class oneEqEddy_LESModel
: public LESModel
{
  
public:
  declareType("oneEqEddy");
  
  oneEqEddy_LESModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class dynOneEqEddy_LESModel
: public LESModel
{

public:
  declareType("dynOneEqEddy");
  
  dynOneEqEddy_LESModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class dynSmagorinsky_LESModel
: public LESModel
{
public:
  declareType("dynSmagorinsky");
  
  dynSmagorinsky_LESModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class kOmegaSST_RASModel
: public RASModel
{
public:
  declareType("kOmegaSST");
  
  kOmegaSST_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class kEpsilonBase_RASModel
: public RASModel
{
  
public:
//   declareType("kEpsilon");
  
  kEpsilonBase_RASModel(OpenFOAMCase& c);
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
};




class kEpsilon_RASModel
: public kEpsilonBase_RASModel
{
public:
  declareType("kEpsilon");
  kEpsilon_RASModel(OpenFOAMCase& ofc, const ParameterSet& ps = ParameterSet());
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};




class realizablekEpsilon_RASModel
: public kEpsilonBase_RASModel
{
public:
  declareType("realizableKE");
  realizablekEpsilon_RASModel(OpenFOAMCase& ofc, const ParameterSet& ps = ParameterSet());
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};




class SpalartAllmaras_RASModel
: public RASModel
{
public:
  declareType("SpalartAllmaras");
  
  SpalartAllmaras_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class LEMOSHybrid_RASModel
: public kOmegaSST_RASModel
{
  
public:
  declareType("LEMOSHybrid");
  
  LEMOSHybrid_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class kOmegaSST_LowRe_RASModel
: public kOmegaSST_RASModel
{
public:
  declareType("kOmegaSST_LowRe");
  
  kOmegaSST_LowRe_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class kOmegaSST2_RASModel
: public kOmegaSST_RASModel
{
public:
  declareType("kOmegaSST2");
  
  kOmegaSST2_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
//   virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};


class kOmegaHe_RASModel
: public kOmegaSST_RASModel
{
public:
  declareType("kOmegaHe");
  
  kOmegaHe_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};

class LRR_RASModel
: public RASModel
{
public:
  declareType("LRR");
  
  LRR_RASModel(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet());
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
  inline static ParameterSet defaultParameters() { return ParameterSet(); }
};


class WALE_LESModel
: public LESModel
{
public:
  declareType("WALE");
  
  WALE_LESModel(OpenFOAMCase& c);
  virtual void addFields( OpenFOAMCase& c ) const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const;
};

}

#endif
