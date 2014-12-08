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

void setDecomposeParDict(OFdicts& dictionaries, int np, const std::string& method);

/**
 Manages basic settings in controlDict, fvSchemes, fvSolution, list of fields
 */
class FVNumerics
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (np, int, 1)
    (writeControl, std::string, "timeStep")
    (writeInterval, double, 100.0)
    (writeFormat, std::string, "binary")
    (purgeWrite, int, 10)
    (deltaT, double, 1.0)
    (endTime, double, 1000.0)
    (decompositionMethod, std::string, "scotch")
  )

protected:
  Parameters p_;
  bool isCompressible_;
   
public:
  FVNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  inline bool isCompressible() const { return isCompressible_; }
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



OFDictData::dict diagonalSolverSetup();
OFDictData::dict stdAsymmSolverSetup(double tol=1e-7, double reltol=0.0, int minIter=0);
OFDictData::dict stdSymmSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict smoothSolverSetup(double tol=1e-7, double reltol=0.0, int minIter=0);
OFDictData::dict GAMGSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict GAMGPCGSolverSetup(double tol=1e-7, double reltol=0.0);


class MeshingNumerics
: public FVNumerics
{
public:
  MeshingNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class simpleFoamNumerics
: public FVNumerics
{
public:
  simpleFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class pimpleFoamNumerics
: public FVNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, FVNumerics::Parameters,
    (nCorrectors, int, 2)
    (nOuterCorrectors, int, 1)
    (nNonOrthogonalCorrectors, int, 0)
    (deltaT, double, 1.0)
    (adjustTimeStep, bool, true)
    (maxCo, double, 0.45)
    (maxDeltaT, double, 1.0)
    (forceLES, bool, false)
    (hasCyclics, bool, false)
  )

protected:
  Parameters p_;

public:
  pimpleFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

class potentialFreeSurfaceFoamNumerics
: public FVNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, FVNumerics::Parameters,
    (nCorrectors, int, 2)
    (nOuterCorrectors, int, 1)
    (nNonOrthogonalCorrectors, int, 0)
    (deltaT, double, 1.0)
    (adjustTimeStep, bool, true)
    (maxCo, double, 0.45)
    (maxDeltaT, double, 1.0)
  )

protected:
  Parameters p_;

public:
  potentialFreeSurfaceFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

class simpleDyMFoamNumerics
: public simpleFoamNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, simpleFoamNumerics::Parameters,
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
  CPPX_DEFINE_OPTIONCLASS(Parameters, FVNumerics::Parameters,
      (solverName, std::string, "cavitatingFoam")
  )

protected:
  Parameters p_;

public:
  cavitatingFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};



class interFoamNumerics
: public FVNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, FVNumerics::Parameters,
      (implicitPressureCorrection, bool, false)
  )

protected:
  Parameters p_;
  std::string pname_;
  std::string alphaname_;
public:
  interFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  inline const std::string& pressureFieldName() const { return pname_; }
};

OFDictData::dict stdMULESSolverSetup(double tol=1e-8, double reltol=0.0);

class LTSInterFoamNumerics
: public interFoamNumerics
{
public:
  LTSInterFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
};

class interPhaseChangeFoamNumerics
: public interFoamNumerics
{
public:
  interPhaseChangeFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

class reactingFoamNumerics
: public FVNumerics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, FVNumerics::Parameters,
    (nCorrectors, int, 2)
    (nOuterCorrectors, int, 1)
    (nNonOrthogonalCorrectors, int, 0)
    (deltaT, double, 1.0)
    (adjustTimeStep, bool, true)
    (maxCo, double, 0.45)
    (maxDeltaT, double, 1.0)
    (forceLES, bool, false)
  )

protected:
  Parameters p_;

public:
  reactingFoamNumerics(OpenFOAMCase& c, const Parameters& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

class reactingParcelFoamNumerics
: public reactingFoamNumerics
{
public:
  reactingParcelFoamNumerics(OpenFOAMCase& c, const Parameters& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

class FSIDisplacementExtrapolationNumerics
: public FaNumerics
{
public:
  FSIDisplacementExtrapolationNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class magneticFoamNumerics
: public FVNumerics
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, FVNumerics::Parameters,
//     (nCorrectors, int, 2)
//   )
// 
// protected:
//   Parameters p_;
// 
public:
  magneticFoamNumerics(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

}

#endif // INSIGHT_NUMERICSCASEELEMENTS_H
