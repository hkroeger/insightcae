#ifndef SELECTABLESUBSETPARAMETERPARSER_H
#define SELECTABLESUBSETPARAMETERPARSER_H

#include "parserdatabase.h"

struct SelectableSubsetParameterParser {

    struct Data
            : public ParserDataBase
    {
        typedef boost::fusion::vector2<std::string, ParserDataBase::Ptr> SubsetData;
        typedef std::vector<SubsetData> SubsetListData;

        std::string default_sel;
        SubsetListData value;

        Data ( const SubsetListData& v, const std::string& ds, const std::string& d );

        void cppAddHeader ( std::set<std::string>& headers ) const override;

        std::string cppType ( const std::string& name ) const override;

        std::string cppValueRep ( const std::string&, const std::string& thisscope  ) const override;

        std::string cppTypeDecl ( const std::string& name,
                                  const std::string& thisscope ) const override;

        std::string cppParamType ( const std::string&  ) const override;

        void cppWriteCreateStatement
        (
            std::ostream& os,
            const std::string& name,
            const std::string& thisscope
        ) const override;

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

    declareType("selectablesubset");

    inline static void insertrule ( PDLParserRuleset& ruleset )
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::lit ( "{{" ) >>
                  * ( ruleset.r_identifier >> ruleset.r_parameterdata )
                  >> qi::lit ( "}}" ) >> ruleset.r_identifier >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParserDataBase::Ptr> (
                                 phx::new_<Data> ( qi::_1, qi::_2, qi::_3 )
                              ) ]

            )
        );
    }
};

#endif // SELECTABLESUBSETPARAMETERPARSER_H
