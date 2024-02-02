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

    void cppAddHeader(std::set<std::string>& headers) const override;

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string& ) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;

    void cppWriteCreateStatement(std::ostream& os, const std::string& name,
                                 const std::string& thisscope) const override;

  };

  declareType("matrix");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( qi::int_ >> 'x' >> qi::int_ >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParserDataBase::Ptr>( phx::new_<Data>(qi::_1, qi::_2, qi::_3) ) ]
        |
        ( ('[' >> ('[' >> qi::double_ % ',' >> ']') % ',' >> ']' ) >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParserDataBase::Ptr>( phx::new_<Data>(qi::_1, qi::_2) ) ]

      )
    );
  }
};

#endif // MATRIXPARAMETERPARSER_H
