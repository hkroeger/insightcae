
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
    BC["type"]="inletOutlet";
    BC["inletValue"]=OFDictData::toUniformField(k);
    BC["value"]=OFDictData::toUniformField(k);
}

void uniformIntensityAndLengthScale::setDirichletBC_omega(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
    double omega=sqrt(k)/p().l;
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]=OFDictData::toUniformField(omega);
    BC["value"]=OFDictData::toUniformField(omega);
}

void uniformIntensityAndLengthScale::setDirichletBC_epsilon(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
    double k=3.*pow(uprime, 2)/2.;
    double epsilon=pow(0.09, 3./4.)*pow(k, 1.5)/p().l;
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]=OFDictData::toUniformField(epsilon);
    BC["value"]=OFDictData::toUniformField(epsilon);
}


void uniformIntensityAndLengthScale::setDirichletBC_nuTilda(OFDictData::dict& BC, double U) const
{
    double nutilda=sqrt(1.5)*p().I * U * p().l;
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]=OFDictData::toUniformField(nutilda);
    BC["value"]=OFDictData::toUniformField(nutilda);
}

void uniformIntensityAndLengthScale::setDirichletBC_R(OFDictData::dict& BC, double U) const
{
    double uprime=p().I*U;
    BC["type"]=OFDictData::data("inletOutlet");
    arma::mat R;
    R << uprime/3. << 0 <<  0 << uprime/3. << 0 << uprime/3. << arma::endr;
    BC["inletValue"]=OFDictData::toUniformField(R);
    BC["value"]=OFDictData::toUniformField(R);
}



}


}
