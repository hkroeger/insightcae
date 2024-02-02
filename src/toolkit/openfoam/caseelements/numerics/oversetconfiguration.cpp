#include "oversetconfiguration.h"

namespace insight {

OversetConfiguration::OversetConfiguration(OpenFOAMCase& c, const ParameterSet& ps)
    : cm_(c),
      p_(ps)
{}

void OversetConfiguration::addFields() const
{
    cm_.addField("zoneID",	FieldInfo(scalarField, dimless,     FieldValue({-1}), volField ) );
}

void OversetConfiguration::addIntoDictionaries (
        OFdicts& dictionaries,
        const std::string& pName,
        const std::string& fluxName
        ) const
{
    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");

    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    fvSolution.subDict("PIMPLE")["oversetAdjustPhi"]=p_.oversetAdjustPhi;

    OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
    auto& ovi = fvSchemes.subDict("oversetInterpolation");
    switch (p_.oversetInterpolation)
    {
        case Parameters::cellVolumeWeight:
            ovi["method"]="cellVolumeWeight";
            break;
        case Parameters::inverseDistance:
            ovi["method"]="inverseDistance";
            break;
        case Parameters::leastSquares:
            ovi["method"]="leastSquares";
            break;
        case Parameters::trackingInverseDistance:
            ovi["method"]="trackingInverseDistance";
            break;
    }

    auto& ovis = fvSchemes.subDict("oversetInterpolationSuppressed");
    ovis["grad("+pName+")"]="";
    ovis["surfaceIntegrate("+fluxName+")"]="";

    if (!p_.skipPoissonWallDist)
    {
        OFDictData::dict& wd = fvSchemes.subDict("wallDist");
        wd["method"]="Poisson";

        fvSchemes.subDict("fluxRequired")["yPsi"]="";
        fvSchemes.subDict("laplacianSchemes")["laplacian(yPsi)"] =
                "Gauss linear uncorrected";

        fvSolution.subDict("solvers")["yPsi"]=
                cm_.stdAsymmSolverSetup();
    }
}


} // namespace insight
