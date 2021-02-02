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

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string&) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope) const override;
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



struct dimensionedScalarParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::string dimensionTypeName_, defaultUnit_;
    double value;

    Data(const std::string& dimensionTypeName, const std::string& defaultUnit, double v, const std::string& d);

    std::string cppType(const std::string&name) const override;
    std::string cppParamType(const std::string&name) const override;
    std::string cppValueRep(const std::string&name, const std::string& thisscope) const override;
    void cppWriteCreateStatement
    (
       std::ostream& os,
       const std::string& name,
       const std::string& thisscope
    ) const override;
  };

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "dimensionedScalar",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( ruleset.r_identifier >> ruleset.r_identifier >> qi::double_ >> ruleset.r_description_string )
         [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      ))
    );
  }
};

#endif // DOUBLEPARAMETERPARSER_H
