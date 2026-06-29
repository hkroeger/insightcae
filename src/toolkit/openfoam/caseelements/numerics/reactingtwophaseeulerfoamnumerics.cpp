#include "reactingtwophaseeulerfoamnumerics.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/ofdicts.h"

namespace insight {

defineType(reactingTwoPhaseEulerFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(reactingTwoPhaseEulerFoamNumerics);

reactingTwoPhaseEulerFoamNumerics::reactingTwoPhaseEulerFoamNumerics(
    OpenFOAMCase &c, ParameterSetInput ip)
: FVNumerics( c, ip.forward<Parameters>(), "p_rgh" )
{}

void reactingTwoPhaseEulerFoamNumerics::addIntoDictionaries(OFdicts &dictionaries) const
{
    FVNumerics::addIntoDictionaries(dictionaries);
    setApplicationName(dictionaries, "reactingTwoPhaseEulerFoam");

    dictionaries.lookupDict("system/fvSchemes") +=
        {
            {"divSchemes", OFDictData::dict{
              {"div\\(phi,alpha.*\\)",            "Gauss upwind" },
              {"div\\(phir,alpha.*\\)",           "Gauss vanLeer 1" },

              {"div\\(alphaRhoPhi.*,U.*\\)",      "Gauss linearUpwindV grad(U)"},
              {"div\\(phi.*,U.*\\)",              "Gauss linearUpwindV grad(U)"},

              {"div\\(alphaRhoPhi.*,Yi\\)",       "Gauss upwind"},
              {"div\\(alphaRhoPhi.*,(h|e).*\\)",  "Gauss linearUpwind limitedGrad"},
              {"div\\(alphaRhoPhi.*,K.*\\)",      "Gauss linearUpwind limitedGrad"},
              {"div\\(alphaPhi.*,p\\)",           "Gauss upwind"},

              {"div\\(alphaRhoPhi.*,(k|epsilon).*\\)",  "Gauss upwind"},
              {"div\\(phim,(k|epsilon)m\\)",      "Gauss upwind"},

              {"div\\(\\(\\(\\(alpha.*\\*thermo:rho.*\\)\\*nuEff.*\\)\\*dev2\\(T\\(grad\\(U.*\\)\\)\\)\\)\\)", "Gauss linear"}
          }
        }
    };


    auto pSolver = [&](double mult)
    {
        return
            /*overset_ ?
        OFcase().stdAsymmSolverSetup(1e-7, 0.01*mult)
             :*/
            (
                isGAMGOk()?
                    OFcase().GAMGPCGSolverSetup(1e-7, 0.01*mult)
                           :
                    OFcase().stdSymmSolverSetup(1e-7, 0.01*mult)
                );
    };

    dictionaries.lookupDict("system/fvSolution") +=
        {
            { "solvers", OFDictData::dict{
               { "\"alpha.*\"", OFDictData::dict{
                                      {"nAlphaCorr",  1},
                                      {"nAlphaSubCycles", 3}
                                  }
               },
               {"p_rgh",  pSolver(1.)},
               {"p_rghFinal",  pSolver(0)},


               { "\"U.*\"", OFcase().smoothSolverSetup(1e-8, 0) },

               {"\"(e|h).*\"", OFcase().smoothSolverSetup(1e-12, 0) },

               {"\"(k|epsilon|Theta).*\"", OFcase().smoothSolverSetup(1e-8, 0)},
               { "\"Yi.*\"", OFcase().smoothSolverSetup(1e-6, 0)}
               }
            },
            {
             "PIMPLE", OFDictData::dict{
                 {"nOuterCorrectors",    4},
                 {"nCorrectors",         1},
                 {"nNonOrthogonalCorrectors", 0},
                 {"nEnergyCorrectors",   1},
                 {"faceMomentum",        true}
             }
            },
            {
             "relaxationFactors", OFDictData::dict{
                  {"fields", OFDictData::dict{
                                 {"iDmdt", 0.1}
                             }},
                  {"equations", OFDictData::dict{
                                    {"\".*\"", 0.5}
                                }},
                  }
            }
    };
}


bool reactingTwoPhaseEulerFoamNumerics::isCompressible() const
{
    return true;
}


} // namespace insight
