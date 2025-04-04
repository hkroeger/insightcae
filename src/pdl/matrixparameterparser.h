#ifndef MATRIXPARAMETERPARSER_H
#define MATRIXPARAMETERPARSER_H

#include "parametergenerator.h"

struct MatrixParameterParser
    : public ParameterGenerator
{
    arma::mat value;

    MatrixParameterParser(arma::uword r, arma::uword c, const std::string& d);
    MatrixParameterParser(const std::vector<std::vector<double> >& mat, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

    void cppWriteCreateStatement(
        std::ostream& os,
        const std::string& psvarname ) const override;


  declareType("matrix");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( qi::int_ >> 'x' >> qi::int_ >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<MatrixParameterParser>(
                           qi::_1, qi::_2, qi::_3) ) ]
        |
        ( ('[' >> ('[' >> qi::double_ % ',' >> ']') % ',' >> ']' ) >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<MatrixParameterParser>(
                           qi::_1, qi::_2) ) ]

      )
    );
  }
};

#endif // MATRIXPARAMETERPARSER_H
