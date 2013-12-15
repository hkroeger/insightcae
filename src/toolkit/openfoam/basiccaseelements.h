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

#include <map>

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

OFDictData::dict stdSymmSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict smoothSolverSetup(double tol=1e-7, double reltol=0.0);
OFDictData::dict GAMGSolverSetup(double tol=1e-7, double reltol=0.0);


class simpleFoamNumerics
: public FVNumerics
{
public:
  simpleFoamNumerics(OpenFOAMCase& c);
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};

class FSIDisplacementExtrapolationNumerics
: public FaNumerics
{
public:
  FSIDisplacementExtrapolationNumerics(OpenFOAMCase& c);
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

class MeshMotionBC
{
public:
  MeshMotionBC();
  virtual ~MeshMotionBC();
  
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) =0;
  virtual MeshMotionBC* clone() const =0;
};

class NoMeshMotion
: public MeshMotionBC
{
public:
  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC);
  virtual MeshMotionBC* clone() const;
};

extern NoMeshMotion noMeshMotion;

class CAFSIBC
: public MeshMotionBC
{
public:
  typedef std::map<double, double> RelaxProfile;
  
protected:
  boost::filesystem::path FEMScratchDir_;
  double clipPressure_;
  RelaxProfile relax_;
  
public:
  CAFSIBC(const boost::filesystem::path& FEMScratchDir, double clipPressure, double relax=0.1);
  CAFSIBC(const boost::filesystem::path& FEMScratchDir, double clipPressure, const RelaxProfile& relax);
  virtual ~CAFSIBC();

  virtual bool addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC);
  virtual MeshMotionBC* clone() const;
};

class WallBC
: public BoundaryCondition
{
protected:
  arma::mat wallVelocity_;
  boost::shared_ptr<MeshMotionBC> meshmotion_;
  
public:
  WallBC
  (
    OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, arma::mat wallVelocity=vec3(0,0,0), 
    const MeshMotionBC& meshmotion = noMeshMotion
  );
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
  virtual void addOptionsToBoundaryDict(OFDictData::dict& bndDict) const;
};

}

#endif // INSIGHT_BASICCASEELEMENTS_H
