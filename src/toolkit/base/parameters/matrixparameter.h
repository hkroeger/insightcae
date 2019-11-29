#ifndef MATRIXPARAMETER_H
#define MATRIXPARAMETER_H


#include "base/parameter.h"


namespace insight
{



class MatrixParameter
    : public Parameter
{
public:
    typedef arma::mat value_type;

protected:
    arma::mat value_;

public:
    declareType ( "matrix" );

    MatrixParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    MatrixParameter ( const arma::mat& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    arma::mat& operator() ();
    const arma::mat& operator() () const;

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override;
    void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath ) override;

    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};



}



#endif // MATRIXPARAMETER_H
