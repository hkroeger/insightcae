#include "parametersetgenerator.h"


ParameterSetGenerator::ParameterSetGenerator(
    const SubsetGenerator& o )
    : SubsetGenerator(o)
{}

std::string ParameterSetGenerator::cppTypeName() const
{
    return name;
}

std::string ParameterSetGenerator::cppInsightType() const
{
    return "insight::ParameterSet";
}

void ParameterSetGenerator::writeCppTypeDeclMakeDefaultFunction_populate(std::ostream &os) const
{
    SubsetGenerator::writeCppTypeDeclMakeDefaultFunction_populate(os);

    os << name << ".initialize();\n";
}

void ParameterSetGenerator::writeCppTypeDeclGetSetFunctions(std::ostream &os) const
{
    SubsetGenerator::writeCppTypeDeclGetSetFunctions(os);

    // convert static data into a ParameterSet
    os << "std::unique_ptr<ParameterSet> cloneParameterSet() const override\n"
       << "{ auto p=makeDefault(); set(*p); return p; }\n";

    // clone function
    os << "std::unique_ptr<insight::ParametersBase> clone() const override\n"
       << "{ return std::unique_ptr<"<<name<<">(new "<<name<<"(*this)); }\n";
}
