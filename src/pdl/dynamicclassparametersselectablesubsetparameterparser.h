#ifndef DYNAMICCLASSPARAMETERSSELECTABLESUBSETPARAMETERPARSER_H
#define DYNAMICCLASSPARAMETERSSELECTABLESUBSETPARAMETERPARSER_H

#include "parserdatabase.h"

struct DynamicClassParametersSelectableSubsetParameterParser {


  // static data:  <selected_type>::Parameters
  // dynamic data:  ParameterSet

    struct Data
            : public ParserDataBase {

        typedef boost::fusion::vector2<std::string, ParserDataBase::Ptr> SubsetData;
        typedef std::vector<SubsetData> SubsetListData;

        std::string base_type, default_sel_;

        Data ( const std::string& base,  const std::string& default_sel, const std::string& d );

        void cppAddHeader ( std::set<std::string>& headers ) const override;
        std::string cppType ( const std::string& name ) const override;
        std::string cppTypeDecl(const std::string& name,
                                const std::string& thisscope) const override;
        std::string cppValueRep ( const std::string& name, const std::string& thisscope ) const override;
        std::string cppParamType ( const std::string& name ) const override;

        void cppWriteCreateStatement ( std::ostream& os, const std::string& name,
                                       const std::string& thisscope ) const override;

        void cppWriteSetStatement
        (
            std::ostream& os,
            const std::string& name,
            const std::string& varname,
            const std::string& staticname,
            const std::string& thisscope
        ) const override;

        void cppWriteGetStatement
        (
            std::ostream& os,
            const std::string& name,
            const std::string& varname,
            const std::string& staticname,
            const std::string& thisscope
        ) const override;
    };

    declareType("dynamicclassparameters");

    inline static void insertrule ( PDLParserRuleset& ruleset )
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                        ( ruleset.r_string >> ( (qi::lit("default") >> ruleset.r_string)
                           | (qi::attr(std::string())) ) >> ruleset.r_description_string )
                        [ qi::_val = phx::construct<ParserDataBase::Ptr> (
                                         phx::new_<Data> ( qi::_1, qi::_2, qi::_3) ) ]

                    )
        );
    }
};

#endif // DYNAMICCLASSPARAMETERSSELECTABLESUBSETPARAMETERPARSER_H
