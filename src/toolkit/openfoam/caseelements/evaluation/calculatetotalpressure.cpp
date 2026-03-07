#include "calculatetotalpressure.h"
#include "openfoam/openfoamdict.h"




namespace insight {




defineType(calculateTotalPressure);
addToOpenFOAMCaseElementFactoryTable(calculateTotalPressure);




calculateTotalPressure::calculateTotalPressure(
    OpenFOAMCase &c, ParameterSetInput ip)
    : functionObject(c, ip.forward<Parameters>())
{}




OFDictData::dict calculateTotalPressure::functionObjectDict() const
{
    OFDictData::dict fod;
    fod["type"]="calculateTotalPressure";

    fod["pAmbient"]=OFDictData::dimensionedData(
        "pAmbient", OFDictData::dimension(1, -1, -2), p().pAmbient );
    fod["pName"]=p().pName;
    fod["UName"]=p().UName;

    if (auto*rhoinf=boost::get<Parameters::rho_rhoInf_type>(&p().rho))
    {
        fod["rhoName"]="rhoInf";
        fod["rhoInf"]=rhoinf->density;
    }
    else if (auto*rhofield=boost::get<Parameters::rho_field_type>(&p().rho))
    {
        fod["rhoName"]=rhofield->rhoName;
    }

    return fod;
}




std::set<std::string> calculateTotalPressure::requiredLibraries() const
{
    auto rl=functionObject::requiredLibraries();
    rl.insert("libcalculateTotalPressure.so");
    return rl;
}




} // namespace insight
