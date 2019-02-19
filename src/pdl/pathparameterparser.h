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
