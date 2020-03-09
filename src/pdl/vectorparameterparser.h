#ifndef VECTORPARAMETERPARSER_H
#define VECTORPARAMETERPARSER_H

#include "parserdatabase.h"

struct VectorParameterParser
{
  struct Data
  : public ParserDataBase
  {
    arma::mat value;

    Data(const arma::mat& v, const std::string& d);

    void cppAddHeader(std::set<std::string>& headers) const override;

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string& ) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;
  };


  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "vector",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( "(" >> *qi::double_ >> ")" >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParserDataBase::Ptr>(
             phx::new_<Data>(vec2mat_(qi::_1), qi::_2)
            ) ]
      ))
    );
  }

};

#endif // VECTORPARAMETERPARSER_H
