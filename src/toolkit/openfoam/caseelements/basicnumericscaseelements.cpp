#include "basicnumericscaseelements.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"


#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{


std::vector<int> factors(int n)
{
  // do a prime factorization of n

  std::vector<int> facs;
  int z = 2;

  while (z * z <= n)
  {
      if (n % z == 0)
      {
          facs.push_back(z);
          n /= z;
      }
      else
      {
          z++;
      }
  }

  if (n > 1)
  {
      facs.push_back( n );
  }

  return facs;
}

std::vector<int> combinefactors
(
  std::vector<int> facs,
  const std::tuple<int,int,int>& po
)
{
  // count locked directions (without decomposition)
  int n_lock=0;
  if (std::get<0>(po)<=0.0) n_lock++;
  if (std::get<1>(po)<=0.0) n_lock++;
  if (std::get<2>(po)<=0.0) n_lock++;
  // if less than 3 factors or directions are locked: extend with ones
  int n_add=std::max(3-int(facs.size()), n_lock);
  for (int k=0;k<n_add;k++)
    facs.push_back(1);

  // bring factors into descending order
  sort(facs.begin(), facs.end());
  std::reverse(facs.begin(), facs.end());

  // get initial number which was factored
  double totprod=1.0;
  for (int f: facs) totprod*=double(f);

  double potot=std::get<0>(po)+std::get<1>(po)+std::get<2>(po);
  std::vector<double> pof = list_of
    (double(std::get<0>(po))/potot)
    (double(std::get<1>(po))/potot)
    (double(std::get<2>(po))/potot);

  std::vector<std::size_t> pof_sorti(pof.size());
  std::iota(pof_sorti.begin(), pof_sorti.end(), 0);
  std::sort(pof_sorti.begin(), pof_sorti.end(), [&pof](std::size_t left, std::size_t right)
  {
      return pof[left] > pof[right];
  });

  std::vector<int> nf(3);
  size_t j=0;

  for (size_t i=0; i<3; i++)
  {
    size_t dir_idx=pof_sorti[i];
    double req_frac=pof[dir_idx];
    int cf=facs[j++];
    while (j<facs.size()-(2-i) && (cf>=0.0) && ( (log(cf)/log(totprod)) < req_frac) )
    {
      {
        cf*=facs[j++];
      }
    }
    nf[dir_idx]=cf;
  }

  return nf;
}

void setDecomposeParDict
(
  OFdicts& dictionaries,
  int np,
  const FVNumerics::Parameters::decompositionMethod_type& method,
  const arma::mat& pom
)
{
  std::tuple<int,int,int> po(int(pom(0)), int(pom(1)), int(pom(2)));

  OFDictData::dict& decomposeParDict=dictionaries.addDictionaryIfNonexistent("system/decomposeParDict");

// #warning hack for testing
  std::vector<int> ns=combinefactors(factors(np), po);
  cout<<"decomp "<<np<<": "<<ns[0]<<" "<<ns[1]<<" "<<ns[2]<<endl;
  decomposeParDict["numberOfSubdomains"]=np;

  if (method==FVNumerics::Parameters::hierarchical)
  {
    decomposeParDict["method"]="hierarchical";
  }
  else if (method==FVNumerics::Parameters::simple)
  {
    decomposeParDict["method"]="simple";
  }
  else if (method==FVNumerics::Parameters::scotch)
  {
    decomposeParDict["method"]="scotch";
  }
  else if (method==FVNumerics::Parameters::metis)
  {
    decomposeParDict["method"]="metis";
  }
  else
  {
      throw insight::Exception("setDecomposeParDict: internal error (unhandled decomposition method)!");
  }

  {
    OFDictData::dict coeffs;
    coeffs["n"]=OFDictData::vector3(ns[0], ns[1], ns[2]);
    coeffs["delta"]=0.001;
    coeffs["order"]="xyz";
    decomposeParDict["hierarchicalCoeffs"]=coeffs;
  }

  {
    OFDictData::dict coeffs;
    coeffs["n"]=OFDictData::vector3(ns[0], ns[1], ns[2]);
    coeffs["delta"]=0.001;
    decomposeParDict["simpleCoeffs"]=coeffs;
  }
}

defineType(FVNumerics);

FVNumerics::FVNumerics(OpenFOAMCase& c, const ParameterSet& ps, const std::string& pName)
: OpenFOAMCaseElement(c, "FVNumerics"),
  p_(ps),
  isCompressible_(false),
  pName_(pName)
{
}

void FVNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
//  std::cerr<<"Addd FVN "<<p_.decompWeights<<std::endl;
  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict["deltaT"]=p_.deltaT;
  controlDict["maxCo"]=0.5;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=p_.endTime;

  std::string wfc("UNDEFINED");
  if (p_.writeControl == Parameters::writeControl_type::adjustableRunTime)
  {
      wfc="adjustableRunTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::clockTime)
  {
      wfc="clockTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::cpuTime)
  {
      wfc="cpuTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::runTime)
  {
      wfc="runTime";
  }
  else if (p_.writeControl == Parameters::writeControl_type::timeStep)
  {
      wfc="timeStep";
  }
  controlDict["writeControl"]=wfc;

  controlDict["writeInterval"]=p_.writeInterval;
  controlDict["purgeWrite"]=p_.purgeWrite;

  std::string wfk("UNDEFINED");
  if (p_.writeFormat == Parameters::writeFormat_type::ascii)
  {
      wfk="ascii";
  } else if (p_.writeFormat == Parameters::writeFormat_type::binary)
  {
      wfk="binary";
  }
  controlDict["writeFormat"]=wfk;

  controlDict["writePrecision"]=8;
  if (OFversion()>=600)
  {
    controlDict["writeCompression"]="on";
  }
  else
  {
    controlDict["writeCompression"]="compressed";
  }
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.addListIfNonexistent("libs");
  controlDict.addSubDictIfNonexistent("functions");

  controlDict.getList("libs").insertNoDuplicate( "\"libwriteData.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libconsistentCurveSampleSet.so\"" );

  OFDictData::dict wonow;
  wonow["type"]="writeData";
  wonow["fileName"]="\"wnow\"";
  wonow["fileNameAbort"]="\"wnowandstop\"";
  wonow["outputControl"]="timeStep";
  wonow["outputInterval"]=1;
  controlDict.addSubDictIfNonexistent("functions")["writeData"]=wonow;

  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  fvSolution.addSubDictIfNonexistent("solvers");
  fvSolution.addSubDictIfNonexistent("relaxationFactors");

  // potentialFoam config
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["Phi"]=stdSymmSolverSetup(1e-7, 0.01);

  OFDictData::dict& PF=fvSolution.addSubDictIfNonexistent("potentialFlow");
  PF["nNonOrthogonalCorrectors"]=10;
  PF["pRefCell"]=0;
  PF["pRefValue"]=0.0;
  PF["PhiRefCell"]=0;
  PF["PhiRefValue"]=0.0;

  OFDictData::dict& fvSchemes=dictionaries.addDictionaryIfNonexistent("system/fvSchemes");
  fvSchemes.addSubDictIfNonexistent("ddtSchemes");
  fvSchemes.addSubDictIfNonexistent("gradSchemes");
  fvSchemes.addSubDictIfNonexistent("divSchemes");
  OFDictData::dict& laplacian=fvSchemes.addSubDictIfNonexistent("laplacianSchemes");

  // potentialFoam
  laplacian["laplacian(1,Phi)"]="Gauss linear limited 0.66";

  fvSchemes.addSubDictIfNonexistent("interpolationSchemes");
  fvSchemes.addSubDictIfNonexistent("snGradSchemes");
  fvSchemes.addSubDictIfNonexistent("fluxRequired");

  if (OFversion()>=300)
  {
    OFDictData::dict& wd = fvSchemes.addSubDictIfNonexistent("wallDist");
    wd["method"]="meshWave";
    OFDictData::dict& wd2 = fvSchemes.addSubDictIfNonexistent("patchDist");
    wd2["method"]="meshWave";
  }

  if ( const auto* map = boost::get<Parameters::mapFieldsConfig_map_type>(&p_.mapFieldsConfig) )
    {
      OFDictData::dict& mfd=dictionaries.addDictionaryIfNonexistent("system/mapFieldsDict");

      OFDictData::list patchMapPairs;
      for (const auto& i: map->patchMap)
        {
          patchMapPairs.push_back(i.targetPatch);
          patchMapPairs.push_back(i.sourcePatch);
        }
      mfd["patchMap"]=patchMapPairs;

      OFDictData::list cuttingPatches;
      std::copy(map->cuttingPatches.begin(), map->cuttingPatches.end(), std::back_inserter(cuttingPatches));
      mfd["cuttingPatches"]=cuttingPatches;
    }


  setDecomposeParDict
  (
    dictionaries,
    p_.np,
    p_.decompositionMethod,
    p_.decompWeights
  );
}

std::string FVNumerics::lqGradSchemeIfPossible() const
{
  if ( (OFversion()<220) )
  {
    return "leastSquares";
  }
  else
  {
    if ( OFcase().hasCyclicBC() )
      return "Gauss linear";
    else
      return "pointCellsLeastSquares";
  }
}

void FVNumerics::insertStandardGradientConfig(OFdicts& dictionaries) const
{
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

  std::string bgrads=lqGradSchemeIfPossible();
  grad["default"]=bgrads;
  grad["grad("+pName_+")"]="Gauss linear";

  grad["limitedGrad"]="cellMDLimited "+bgrads+" 1";

  grad["grad(omega)"]="cellLimited "+bgrads+" 1";
  grad["grad(epsilon)"]="cellLimited "+bgrads+" 1";
  grad["grad(k)"]="cellLimited "+bgrads+" 1";
  grad["grad(nuTilda)"]="cellLimited "+bgrads+" 1";
}

bool FVNumerics::isUnique() const
{
  return true;
}



defineType(potentialFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(potentialFoamNumerics);

potentialFoamNumerics::potentialFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0., 0., 0.}), volField ) );
}


void potentialFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="potentialFoam";


  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
//  solvers["Phi"]=GAMGPCGSolverSetup(1e-7, 0.01);
  solvers["p"]=GAMGPCGSolverSetup(1e-7, 0.01);


//  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("potentialFlow");
//  SIMPLE["nNonOrthogonalCorrectors"]=10;
//  SIMPLE["pRefCell"]=0;
//  SIMPLE["pRefValue"]=0.0;
//  SIMPLE["PhiRefCell"]=0;
//  SIMPLE["PhiRefValue"]=0.0;


  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";

  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");


  grad["default"]="Gauss linear";

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="none";
  div["div(phi,U)"]	=	"bounded Gauss linear";
  div["div(div(phi,U))"]	=	"Gauss linear";


  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear corrected";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="corrected";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}

ParameterSet potentialFoamNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}






defineType(laplacianFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(laplacianFoamNumerics);

laplacianFoamNumerics::laplacianFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("T", FieldInfo(vectorField, 	dimTemperature, 		FieldValue({p_.Tinternal}), volField ) );
}


void laplacianFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="laplacianFoam";


  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
//  solvers["Phi"]=GAMGPCGSolverSetup(1e-7, 0.01);
  solvers["T"]=GAMGPCGSolverSetup(1e-7, 0.0);


//  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("potentialFlow");
//  SIMPLE["nNonOrthogonalCorrectors"]=10;
//  SIMPLE["pRefCell"]=0;
//  SIMPLE["pRefValue"]=0.0;
//  SIMPLE["PhiRefCell"]=0;
//  SIMPLE["PhiRefValue"]=0.0;


  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";

  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="cellLimited pointCellsLeastSquares";

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="none";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.75";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.75";

//  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
//  fluxRequired["default"]="no";
//  fluxRequired["p"]="";

  // ============ setup controlDict ================================

  OFDictData::dict& tp=dictionaries.lookupDict("constant/transportProperties");
  tp["DT"]=p_.DT;

}

ParameterSet laplacianFoamNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}



FaNumerics::FaNumerics(OpenFOAMCase& c, const ParameterSet& p)
: OpenFOAMCaseElement(c, "FaNumerics"), p_(p)
{
}

void FaNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& faSolution=dictionaries.addDictionaryIfNonexistent("system/faSolution");
  faSolution.addSubDictIfNonexistent("solvers");
  faSolution.addSubDictIfNonexistent("relaxationFactors");

  OFDictData::dict& faSchemes=dictionaries.addDictionaryIfNonexistent("system/faSchemes");
  faSchemes.addSubDictIfNonexistent("ddtSchemes");
  faSchemes.addSubDictIfNonexistent("gradSchemes");
  faSchemes.addSubDictIfNonexistent("divSchemes");
  faSchemes.addSubDictIfNonexistent("laplacianSchemes");
  faSchemes.addSubDictIfNonexistent("interpolationSchemes");
  faSchemes.addSubDictIfNonexistent("snGradSchemes");
  faSchemes.addSubDictIfNonexistent("fluxRequired");
}


bool FaNumerics::isUnique() const
{
  return true;
}



tetFemNumerics::tetFemNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "tetFemNumerics")
{
}

void tetFemNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& tetFemSolution=dictionaries.addDictionaryIfNonexistent("system/tetFemSolution");
  tetFemSolution.addSubDictIfNonexistent("solvers");
}

bool tetFemNumerics::isUnique() const
{
  return true;
}



OFDictData::dict diagonalSolverSetup()
{
  OFDictData::dict d;
  d["solver"]="diagonal";
  return d;
}

OFDictData::dict stdAsymmSolverSetup(double tol, double reltol, int minIter)
{
  OFDictData::dict d;
  d["solver"]="PBiCG";
  d["preconditioner"]="DILU";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  if (minIter) d["minIter"]=minIter;
  return d;
}

OFDictData::dict stdSymmSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="PCG";
  d["preconditioner"]="DIC";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  return d;
}

OFDictData::dict GAMGSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="GAMG";
  d["minIter"]=1;
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["smoother"]="DICGaussSeidel";
  d["nPreSweeps"]=2;
  d["nPostSweeps"]=2;
  d["cacheAgglomeration"]="on";
  d["agglomerator"]="faceAreaPair";
  d["nCellsInCoarsestLevel"]=500;
  d["mergeLevels"]=1;
  return d;
}

OFDictData::dict GAMGPCGSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="PCG";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["minIter"]=1;
  OFDictData::dict pd;
  pd["preconditioner"]="GAMG";
  pd["smoother"]="DICGaussSeidel";
  pd["nPreSweeps"]=2;
  pd["nPostSweeps"]=2;
  pd["cacheAgglomeration"]="on";
  pd["agglomerator"]="faceAreaPair";
  pd["nCellsInCoarsestLevel"]=10;
  pd["mergeLevels"]=1;
  d["preconditioner"]=pd;
  return d;
}

OFDictData::dict smoothSolverSetup(double tol, double reltol, int minIter)
{
  OFDictData::dict d;
  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["nSweeps"]=1;
  if (minIter) d["minIter"]=minIter;
  return d;
}




defineType(MeshingNumerics);
addToOpenFOAMCaseElementFactoryTable(MeshingNumerics);

MeshingNumerics::MeshingNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p")
{
//  std::cerr<<"Constr. MN "<<p_.decompWeights<<std::endl;
}


void MeshingNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["writeFormat"]="ascii";
  if (OFversion()>=600)
  {
    controlDict["writeCompression"]="on";
  }
  else
  {
    controlDict["writeCompression"]="compressed";
  }
  controlDict["application"]="none";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1.0;
}


ParameterSet MeshingNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}

}
