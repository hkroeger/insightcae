#ifndef CADSKETCHPARAMETERPARSER_H
#define CADSKETCHPARAMETERPARSER_H


#include "parserdatabase.h"

struct CADSketchParameterParser
{
    struct Data
        : public ParserDataBase
    {
        std::string
            script_,
            makeDefaultParametersFunctionName_,
            makeDefaultLayerParametersFunctionName_,
            sketchAppearanceFunctionName_;

        std::vector<boost::fusion::vector2<int, std::string> > references_;

        Data(
            const std::string& script,
            const std::string& makeDefaultParametersFunctionName,
            const std::string& makeDefaultLayerParametersFunctionName,
            const std::string& sketchAppearanceFunctionName,
            const std::vector<boost::fusion::vector2<int, std::string> >& references,
            const std::string& d);

        std::string refParameter() const;

        void cppAddHeader(std::set< std::string >& headers) const override;

        std::string cppType(const std::string&) const override;

        std::string cppParamType(const std::string& ) const override;

        std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;

        std::string cppConstructorParameters(const std::string &name,
                                             const std::string& thisscope) const override;

        void cppWriteSetStatement
            (
                std::ostream& os,
                const std::string&,
                const std::string& varname,
                const std::string& staticname,
                const std::string&
                ) const override;


        void cppWriteGetStatement
            (
                std::ostream& os,
                const std::string&,
                const std::string& varname,
                const std::string& staticname,
                const std::string&
                ) const override;

    };

    declareType("cadsketch");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(
                    (
                       ruleset.r_string
                    >> ruleset.r_string
                    >> ruleset.r_string
                    >> ruleset.r_string
                    >> ( ( qi::lit("references") >> *( qi::int_ > '=' > ruleset.r_string ) )
                        | qi::attr(std::vector<boost::fusion::vector2<int, std::string> >()) )
                    >> ruleset.r_description_string
                    )
                    [ qi::_val = phx::construct<ParserDataBase::Ptr>(
                         phx::new_<Data>(
                             qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6)) ]
                    )
                );
    }
};

#endif // CADSKETCHPARAMETERPARSER_H
