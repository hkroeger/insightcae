#ifndef LABELEDARRAYKEYSELECTIONGENERATOR_H
#define LABELEDARRAYKEYSELECTIONGENERATOR_H

#include "parametergenerator.h"

struct LabeledArrayKeySelectionGenerator : public ParameterGenerator
{
    std::string arrayParameterPath;
    std::string defaultSelection;

    LabeledArrayKeySelectionGenerator(
        const std::string& arrayPath,
        const std::string& defSel,
        const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

    void cppWriteCreateStatement(
        std::ostream& os,
        const std::string& psvarname) const override;

    // cppWriteSetStatement  — base is correct: varname.set(staticname)
    // cppWriteGetStatement  — base is correct: staticname = varname(); staticname.setPath(...)

    declareType("labeledArrayKeySelection");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add(
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(
                (ruleset.r_string >> ruleset.r_string >> ruleset.r_description_string)
                [qi::_val = phx::construct<ParameterGeneratorPtr>(
                    phx::new_<LabeledArrayKeySelectionGenerator>(
                        qi::_1, qi::_2, qi::_3))]
            )
        );
    }
};

#endif // LABELEDARRAYKEYSELECTIONGENERATOR_H
