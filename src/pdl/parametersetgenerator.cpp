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

void ParameterSetGenerator::writeCppTypeDeclGetSetFunctions(
    std::ostream &os,
    CodeMode mode, const std::string& qual ) const
{
    SubsetGenerator::writeCppTypeDeclGetSetFunctions(os, mode, qual);

    if (
        base_types_ &&
        (std::find_if( base_types_->begin(), base_types_->end(),
                        [](const BaseType& bt)
                        { return !boost::fusion::at_c<0>(bt); } )
            !=base_types_->end())
        )
    {
        switch (mode)
        {
        case CodeMode::Inline:
            // convert static data into a ParameterSet
            os << "std::unique_ptr<ParameterSet> cloneParameterSet() const override\n"
               << "{ auto p=makeDefault(); set(*p); return ParameterSet::finalize(std::move(p)); }\n";

            // clone function
            os << "std::unique_ptr<insight::ParametersBase> clone() const override\n"
               << "{ return std::unique_ptr<"<<name<<">(new "<<name<<"(*this)); }\n";
            break;

        case CodeMode::Declaration:
            os << "std::unique_ptr<insight::ParameterSet> cloneParameterSet() const override;\n"
               << "std::unique_ptr<insight::ParametersBase> clone() const override;\n";
            break;

        case CodeMode::Definition:
            os << "std::unique_ptr<insight::ParameterSet> "<<qual<<"cloneParameterSet() const\n"
               << "{ auto p=makeDefault(); set(*p); return ParameterSet::finalize(std::move(p)); }\n";

            os << "std::unique_ptr<insight::ParametersBase> "<<qual<<"clone() const\n"
               << "{ return std::unique_ptr<"<<name<<">(new "<<name<<"(*this)); }\n";
            break;
        }
    }
}
