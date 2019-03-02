#ifndef MATRIXPARAMETERPARSER_H
#define MATRIXPARAMETERPARSER_H

#include "parserdatabase.h"

struct MatrixParameterParser
{
  struct Data
  : public ParserDataBase
  {
    arma::mat value;

    Data(arma::uword r, arma::uword c, const std::string& d);
    Data(const std::vector<std::vector<double> >& mat, const std::string& d);

    virtual void cppAddHeader(std::set<std::string>& headers) const;

    virtual std::string cppType(const std::string&) const;

    virtual std::string cppParamType(const std::string& ) const;

    virtual std::string cppValueRep(const std::string& ) const;

    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const;

  };

  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "matrix",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
        ( qi::int_ >> 'x' >> qi::int_ >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParserDataBase::Ptr>( phx::new_<Data>(qi::_1, qi::_2, qi::_3) ) ]
        |
        ( ('[' >> ('[' >> qi::double_ % ',' >> ']') % ',' >> ']' ) >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParserDataBase::Ptr>( phx::new_<Data>(qi::_1, qi::_2) ) ]
      ))
    );
  }
};

#endif // MATRIXPARAMETERPARSER_H
