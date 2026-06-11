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

    // os << name << ".initialize();\n";
}

void ParameterSetGenerator::writeCppTypeDeclGetSetFunctions(std::ostream &os) const
{
    SubsetGenerator::writeCppTypeDeclGetSetFunctions(os);

    if (
        base_types_ &&
        (std::find_if( base_types_->begin(), base_types_->end(),
                        [](const BaseType& bt)
                        { return !boost::fusion::at_c<0>(bt); } )
            !=base_types_->end())
        )
    {
        // convert static data into a ParameterSet
        os << "std::unique_ptr<ParameterSet> cloneParameterSet() const override\n"
           << "{ auto p=makeDefault(); set(*p); return ParameterSet::finalize(std::move(p)); }\n";

        // clone function
        os << "std::unique_ptr<insight::ParametersBase> clone() const override\n"
           << "{ return std::unique_ptr<"<<name<<">(new "<<name<<"(*this)); }\n";
    }
}
