/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
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

#ifndef INSIGHT_NUMERICSCASEELEMENTS_H
#define INSIGHT_NUMERICSCASEELEMENTS_H

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
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
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

}

#endif // INSIGHT_NUMERICSCASEELEMENTS_H
