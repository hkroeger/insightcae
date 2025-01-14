#ifndef PARAMETERSETGENERATOR_H
#define PARAMETERSETGENERATOR_H

#include "subsetgenerator.h"

struct ParameterSetGenerator
    : public SubsetGenerator
{
public:
    ParameterSetGenerator(const SubsetGenerator& o);

    std::string cppTypeName() const override;
    std::string cppInsightType() const override;

    void writeCppTypeDeclMakeDefaultFunction_populate(std::ostream &os) const override;

    void writeCppTypeDeclGetSetFunctions(
        std::ostream& os ) const override;
};

#endif // PARAMETERSETGENERATOR_H
