#ifndef DOUBLERANGEPARAMETERPARSER_H
#define DOUBLERANGEPARAMETERPARSER_H

#include "parserdatabase.h"

struct DoubleRangeParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::vector<double> value;

    Data(const std::vector<double>& v, const std::string& d);

    void cppAddHeader(std::set<std::string>& headers) const override;

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string&) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope) const override;

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
      "doubleRange",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
        new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
            ( "(" >> *qi::double_ >> ")" >> ruleset.r_description_string )
            [ qi::_val = phx::construct<ParserDataBase::Ptr>(
               phx::new_<Data>(qi::_1, qi::_2)
              ) ]
      ))
    );
  }
};



#endif // DOUBLERANGEPARAMETERPARSER_H
