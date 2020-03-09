#ifndef PATHPARAMETERPARSER_H
#define PATHPARAMETERPARSER_H



#include "parserdatabase.h"

struct PathParameterParser
{
  struct Data
  : public ParserDataBase
  {
    boost::filesystem::path value;

    Data(const boost::filesystem::path& v, const std::string& d);

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

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "path",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( ruleset.r_string >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};

#endif // PATHPARAMETERPARSER_H
