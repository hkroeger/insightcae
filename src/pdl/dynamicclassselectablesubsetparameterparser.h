#ifndef DYNAMICCLASSSELECTABLESUBSETPARAMETERPARSER_H
#define DYNAMICCLASSSELECTABLESUBSETPARAMETERPARSER_H

#include "parametergenerator.h"


struct DynamicClassSelectableSubsetParameterParser
    : public ParameterGenerator
{

        typedef boost::fusion::vector2<std::string, ParameterGeneratorPtr> SubsetData;
        typedef std::vector<SubsetData> SubsetListData;

        std::string base_type, default_sel_;

        DynamicClassSelectableSubsetParameterParser (
            const std::string& base,  const std::string& default_sel, const std::string& d );

        void cppAddRequiredInclude ( std::set<std::string>&  ) const override;

        std::string cppInsightType (  ) const override;
        std::string cppStaticType () const override;
        std::string cppDefaultValueExpression ( ) const override;

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

    declareType("dynamicclassconfig");

    inline static void insertrule ( PDLParserRuleset& ruleset )
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( ruleset.r_string
                 >> ( (qi::lit("default")
                      >> ruleset.r_string)|(qi::attr(std::string())) )
                 >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParameterGeneratorPtr> (
                         phx::new_<DynamicClassSelectableSubsetParameterParser> (
                             qi::_1, qi::_2, qi::_3) ) ]

            )
        );
    }
};



#endif // DYNAMICCLASSSELECTABLESUBSETPARAMETERPARSER_H
