#ifndef SELECTIONPARAMETERPARSER_H
#define SELECTIONPARAMETERPARSER_H

#include "parserdatabase.h"

struct SelectionParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::vector<std::string> selections;
    std::string selection;

    Data(const std::vector<std::string>& sels, const std::string& sel, const std::string& d);

    std::string cppType(const std::string&) const override;
    std::string cppTypeDecl(const std::string& name,
                            const std::string& thisscope) const override;
    std::string cppValueRep(const std::string& name, const std::string& thisscope ) const override;

    std::string cppParamType(const std::string& ) const override;

    void cppWriteCreateStatement(std::ostream& os, const std::string& name,
                                 const std::string& thisscope) const override;

    void cppWriteSetStatement(
        std::ostream& os,
        const std::string&,
        const std::string& varname,
        const std::string& staticname,
        const std::string&
    ) const override;

    void cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& thisscope) const override;

  };

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "selection",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(
       new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( "(" >> *(ruleset.r_identifier) >> ")" >> ruleset.r_identifier >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2, qi::_3)) ]
      ))
    );
  }
};

#endif // SELECTIONPARAMETERPARSER_H
