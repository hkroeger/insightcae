#include "compressibleinterfoamnumerics.h"

namespace insight
{



defineType(compressibleInterFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(compressibleInterFoamNumerics);



compressibleInterFoamNumerics::compressibleInterFoamNumerics(
    OpenFOAMCase& c,
    ParameterSetInput ip )
    : interFoamNumerics(c, ip.forward<Parameters>())
{
    OFcase().addField("T", 	FieldInfo(scalarField, dimTemperature, FieldValue({p().Tinternal}), volField ) );
}




void compressibleInterFoamNumerics::addIntoDictionaries ( OFdicts& dictionaries ) const
{
    interFoamNumerics::addIntoDictionaries(dictionaries);

    setApplicationName(dictionaries, "compressibleInterFoam");

    OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
    OFDictData::dict& div=fvSchemes.subDict("divSchemes");
    div["div(rhoPhi,T)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(T)");
    div["div(rhoPhi,K)"]		= "Gauss upwind";

    div["div(phi,thermo:rho."+phaseNames().first+")"]   = "Gauss upwind";
    div["div(phi,thermo:rho."+phaseNames().second+")"]	= "Gauss upwind";
    div["div(phi,p)"]                   = "Gauss upwind";
    div["div((phi+meshPhi),p)"]         = "Gauss upwind";

    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");

    for (
        const auto& f:
        std::map<std::string,double>(
            {{"", 1.}, {"Final", 0.}}
            ) )
    {
        solvers["T"+f.first]=OFcase().smoothSolverSetup(1e-8, 0.1*f.second);
    }

    for ( const std::string& qty:
         {"k", "epsilon", "omega", "nuTilda", "R"} )
    {
        div["div(rhoPhi,"+qty+")"]=div["div(phi,"+qty+")"];
        div.erase("div(phi,"+qty+")");
    }
}




bool compressibleInterFoamNumerics::isCompressible() const
{
    return true;
}




}
