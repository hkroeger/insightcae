#ifndef LABELEDARRAYGENERATOR_H
#define LABELEDARRAYGENERATOR_H

#include "parametergenerator.h"


struct LabeledArrayGenerator
    : public ParameterGenerator
{
        std::string patternOrKeysPath;
        enum ArgType { Pattern, KeysPath} argType;

        ParameterGeneratorPtr value;
        size_t num;

        LabeledArrayGenerator (
            const std::string& pok,
            ArgType at,
            ParameterGeneratorPtr v,
            size_t n,
            const std::string& d );

        void setName(const std::string& name) override;

        void cppAddRequiredInclude(std::set<std::string>& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;

        void writeCppTypeDecl (std::ostream& os ) const override;

        void cppWriteCreateStatement
            (
                std::ostream& os,
            const std::string& psvarname
                ) const override;

        void cppWriteSetStatement
            (
                std::ostream& os,
                const std::string& varname,
                const std::string& staticname
                ) const override;

        void cppWriteGetStatement
            (
                std::ostream& os,
                const std::string& varname,
                const std::string& staticname
                ) const override;

    declareType("labeledarray");

    inline static void insertrule ( PDLParserRuleset& ruleset )
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::lit("keysFrom") >> ruleset.r_string
                 >> '[' >> ruleset.r_parameterdata >> ']'
                 >> '*' >> qi::int_ >> ruleset.r_description_string )
                    [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                         phx::new_<LabeledArrayGenerator>(
                             qi::_1, LabeledArrayGenerator::KeysPath,
                             qi::_2, qi::_3, qi::_4)) ]
                |
                ( ruleset.r_string
                 >> '[' >> ruleset.r_parameterdata >> ']'
                 >> '*' >> qi::int_ >> ruleset.r_description_string )
                    [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                         phx::new_<LabeledArrayGenerator>(
                             qi::_1, LabeledArrayGenerator::Pattern,
                             qi::_2, qi::_3, qi::_4)) ]
                )
            );
    }
};
#endif // LABELEDARRAYGENERATOR_H
