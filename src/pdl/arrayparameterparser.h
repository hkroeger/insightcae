#ifndef ARRAYPARAMETERPARSER_H
#define ARRAYPARAMETERPARSER_H

#include "parserdatabase.h"

struct ArrayParameterParser
{
  struct Data
  : public ParserDataBase
  {
    ParserDataBase::Ptr value;
    size_t num;

    Data(ParserDataBase::Ptr v, size_t n, const std::string& d);

    virtual void cppAddHeader(std::set<std::string>& headers) const;

    virtual std::string cppValueRep(const std::string& name) const;

    virtual std::string cppType(const std::string& name) const;

    virtual std::string cppTypeDecl ( const std::string& name ) const;

    virtual std::string cppParamType(const std::string& ) const;

    virtual void cppWriteCreateStatement
    (
        std::ostream& os,
        const std::string& name
    ) const;

    virtual void cppWriteSetStatement
    (
        std::ostream& os,
        const std::string& name,
        const std::string& varname,
        const std::string& staticname,
        const std::string& thisscope
    ) const;

    virtual void cppWriteGetStatement
    (
        std::ostream& os,
        const std::string& name,
        const std::string& varname,
        const std::string& staticname,
        const std::string& thisscope
    ) const;

  };

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "array",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( '[' >> ruleset.r_parameterdata >> ']' >> '*' >> qi::int_ >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2, qi::_3)) ]
      ))
    );
  }
};


#endif // ARRAYPARAMETERPARSER_H
