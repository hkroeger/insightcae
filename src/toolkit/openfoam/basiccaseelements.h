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
#include "openfoam/openfoamcase.h"

namespace insight 
{


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


OFDictData::dict smoothSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict GAMGSolverSetup(double tol=1e-7, double reltol=0.0);


class simpleFoamNumerics
: public FVNumerics
{
public:
  simpleFoamNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class transportModel
: public OpenFOAMCaseElement
{
public:
  transportModel(OpenFOAMCase& c);
};


class singlePhaseTransportProperties
: public transportModel
{
public:
  singlePhaseTransportProperties(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


class turbulenceModel
: public OpenFOAMCaseElement
{
public:
  turbulenceModel(OpenFOAMCase& c);
};


class laminar_RASModel
: public turbulenceModel
{
public:
  laminar_RASModel(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
};


class kOmegaSST_RASModel
: public turbulenceModel
{
public:
  kOmegaSST_RASModel(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;  
};



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


class SuctionInletBC
: public BoundaryCondition
{
  double pressure_;
public:
  SuctionInletBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, double pressure=0.0);
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};


class WallBC
: public BoundaryCondition
{
protected:
  arma::mat wallVelocity_;
  
public:
  WallBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, arma::mat wallVelocity=vec3(0,0,0));
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
  virtual void addOptionsToBoundaryDict(OFDictData::dict& bndDict) const;
};

}

#endif // INSIGHT_BASICCASEELEMENTS_H
