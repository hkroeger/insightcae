
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"

#include <memory>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;


namespace insight
{


namespace turbulenceBC
{

defineType(turbulenceBC);
defineDynamicClass(turbulenceBC);


turbulenceBC::turbulenceBC(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}

turbulenceBC::~turbulenceBC()
{
}



defineType(uniformIntensityAndLengthScale);
addToFactoryTable(turbulenceBC, uniformIntensityAndLengthScale);
addToStaticFunctionTable(turbulenceBC, uniformIntensityAndLengthScale, defaultParameters);


uniformIntensityAndLengthScale::uniformIntensityAndLengthScale(ParameterSetInput ip)
    : turbulenceBC(ip.forward<Parameters>())
{
}


void uniformIntensityAndLengthScale::setDirichletBC_k(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
//     BC["type"]="fixedValue";
//     BC["value"]="uniform "+lexical_cast<string>(k);
    BC["type"]="inletOutlet";
    BC["inletValue"]="uniform "+lexical_cast<string>(k);
    BC["value"]="uniform "+lexical_cast<string>(k);
}

void uniformIntensityAndLengthScale::setDirichletBC_omega(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
    double omega=sqrt(k)/p().l;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform "+lexical_cast<string>(omega);
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform "+lexical_cast<string>(omega);
    BC["value"]="uniform "+lexical_cast<string>(omega);
}

void uniformIntensityAndLengthScale::setDirichletBC_epsilon(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
    double k=3.*pow(uprime, 2)/2.;
    double epsilon=pow(0.09, 3./4.)*pow(k, 1.5)/p().l;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform "+lexical_cast<string>(epsilon);
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform "+lexical_cast<string>(epsilon);
    BC["value"]="uniform "+lexical_cast<string>(epsilon);
}


void uniformIntensityAndLengthScale::setDirichletBC_nuTilda(OFDictData::dict& BC, double U) const
{
    double nutilda=sqrt(1.5)*p().I * U * p().l;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform "+lexical_cast<string>(nutilda);
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform "+lexical_cast<string>(nutilda);
    BC["value"]="uniform "+lexical_cast<string>(nutilda);
}

void uniformIntensityAndLengthScale::setDirichletBC_R(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform ("+lexical_cast<string>(uprime/3.)+" 0 0 "+lexical_cast<string>(uprime/3.)+" 0 "+lexical_cast<string>(uprime/3.)+")";
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform ("+lexical_cast<string>(uprime/3.)+" 0 0 "+lexical_cast<string>(uprime/3.)+" 0 "+lexical_cast<string>(uprime/3.)+")";
    BC["value"]="uniform ("+lexical_cast<string>(uprime/3.)+" 0 0 "+lexical_cast<string>(uprime/3.)+" 0 "+lexical_cast<string>(uprime/3.)+")";
}



}


}
