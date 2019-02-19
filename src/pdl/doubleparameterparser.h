#ifndef DOUBLEPARAMETERPARSER_H
#define DOUBLEPARAMETERPARSER_H

#include "parserdatabase.h"

struct DoubleParameterParser
{
  struct Data
  : public ParserDataBase
  {
    double value;

    Data(double v, const std::string& d);

    virtual std::string cppType(const std::string&) const;

    virtual std::string cppParamType(const std::string&) const;

    virtual std::string cppValueRep(const std::string&) const;
  };

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "double",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( qi::double_ >> ruleset.r_description_string )
         [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};

#endif // DOUBLEPARAMETERPARSER_H
