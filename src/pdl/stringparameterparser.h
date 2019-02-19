#ifndef STRINGPARAMETERPARSER_H
#define STRINGPARAMETERPARSER_H

#include "parserdatabase.h"

struct StringParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::string value;

    Data(const std::string& v, const std::string& d);

    virtual void cppAddHeader(std::set< std::string >& headers) const;

    virtual std::string cppType(const std::string&) const;

    virtual std::string cppParamType(const std::string& ) const;

    virtual std::string cppValueRep(const std::string& ) const;

  };

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "string",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( ruleset.r_string >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};

#endif // STRINGPARAMETERPARSER_H
