#ifndef DYNAMICCLASSSELECTABLESUBSETPARAMETERPARSER_H
#define DYNAMICCLASSSELECTABLESUBSETPARAMETERPARSER_H

#include "parserdatabase.h"


struct DynamicClassSelectableSubsetParameterParser {

    struct Data
            : public ParserDataBase {

        typedef boost::fusion::vector2<std::string, ParserDataBase::Ptr> SubsetData;
        typedef std::vector<SubsetData> SubsetListData;

        std::string base_type, default_sel_;

        Data ( const std::string& base,  const std::string& default_sel, const std::string& d );

        virtual void cppAddHeader ( std::set<std::string>&  ) const;
        virtual std::string cppType ( const std::string&  ) const;
        virtual std::string cppValueRep ( const std::string&  ) const;
        virtual std::string cppParamType ( const std::string&  ) const;

        virtual void cppWriteCreateStatement
        (
            std::ostream& os,
            const std::string& name
        ) const;

        virtual void cppWriteSetStatement
        (
            std::ostream& os,
            const std::string& ,
            const std::string& varname,
            const std::string& staticname,
            const std::string&
        ) const;

        virtual void cppWriteGetStatement
        (
            std::ostream& os,
            const std::string& ,
            const std::string& varname,
            const std::string& staticname,
            const std::string&
        ) const;
    };


    template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
    inline static void insertrule ( PDLParserRuleset<Iterator,Skipper>& ruleset )
    {
        ruleset.parameterDataRules.add
        (
            "dynamicclassconfig",
            typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr (
                      new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule (
                        ( ruleset.r_string >> ( (qi::lit("default") >> ruleset.r_string)|(qi::attr(std::string())) ) >> ruleset.r_description_string )
                        [ qi::_val = phx::construct<ParserDataBase::Ptr> ( phx::new_<Data> ( qi::_1, qi::_2, qi::_3) ) ]
                    ) )
        );
    }
};



#endif // DYNAMICCLASSSELECTABLESUBSETPARAMETERPARSER_H
