#ifndef INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H
#define INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H


#include "base/parameter.h"
#include "base/units.h"


namespace insight {



template<class T, class Dim, char const* N>
class SimpleDimensionedParameter
    : public Parameter
{

public:
    typedef T base_value_type;
    typedef Dim dimension_type;
    typedef boost::units::quantity<dimension_type, base_value_type> value_type;

protected:
    boost::units::quantity<dimension_type, double> defaultUnit_;
    value_type value_;

public:
    declareType ( N );

    SimpleDimensionedParameter
    (
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
    )
        : Parameter ( description, isHidden, isExpert, isNecessary, order )
    {}

    SimpleDimensionedParameter
    (
        const value_type& value,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
    )
        : Parameter ( description, isHidden, isExpert, isNecessary, order ),
          value_ ( value ),
          defaultUnit_(1.0*dimension_type())
    {}

    SimpleDimensionedParameter
    (
        const base_value_type& value, const boost::units::quantity<dimension_type, double>& defaultUnit,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
    )
        : Parameter ( description, isHidden, isExpert, isNecessary, order ),
          defaultUnit_(defaultUnit),
          value_ ( value_type::from_value(value*defaultUnit_.value()) )
    {}

    virtual value_type& operator() ()
    {
        return value_;
    }
    virtual const value_type& operator() () const
    {
        return value_;
    }

    void setInDefaultUnit(const base_value_type& newValue)
    {
      value_ = value_type::from_value( newValue * defaultUnit_.value() );
    }

    base_value_type getInDefaultUnit() const
    {
      return value_.value()/defaultUnit_.value();
    }

    std::string latexRepresentation() const override
    {
        return SimpleLatex( valueToString ( value_.value() ) + boost::units::symbol_string(dimension_type()) ).toLaTeX();
    }

    std::string plainTextRepresentation(int /*indent*/=0) const override
    {
        return SimpleLatex( valueToString ( value_.value() ) + boost::units::symbol_string(dimension_type()) ).toPlainText();
    }


    Parameter* clone() const override
    {
      using namespace boost::units;
        return new SimpleDimensionedParameter<T, Dim, N>
            (
              value_.value()/defaultUnit_.value(), defaultUnit_,
              description_.simpleLatex(),
              isHidden_, isExpert_, isNecessary_, order_
            );
    }

    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override
    {
        using namespace rapidxml;
        xml_node<>* child = Parameter::appendToNode ( name, doc, node, inputfilepath );
        base_value_type nv = value_.value()/defaultUnit_.value();
        child->append_attribute
        (
            doc.allocate_attribute
            (
                "value",
                doc.allocate_string ( valueToString ( nv ).c_str() )
            )
        );
        return child;
    }

    void readFromNode
    (
        const std::string& name,
        rapidxml::xml_document<>&,
        rapidxml::xml_node<>& node,
        boost::filesystem::path
    ) override
    {
        using namespace rapidxml;
      using namespace boost::units;
        xml_node<>* child = findNode ( node, name, type() );
        if ( child )
        {
          auto valueattr=child->first_attribute ( "value" );
          insight::assertion(valueattr, "No value attribute present in "+name+"!");
          base_value_type nv;
          stringToValue ( valueattr->value(), nv );
          value_ = value_type::from_value(defaultUnit_.value() * nv);
        }
        else
        {
          insight::Warning(
                boost::str(
                  boost::format(
                   "No xml node found with type '%s' and name '%s', default value '%s' is used."
                   ) % type() % name % plainTextRepresentation()
                 )
              );
        }
    }

    void reset(const Parameter& p) override
    {
      if (const auto* op = dynamic_cast<const SimpleDimensionedParameter<T,Dim,N>*>(&p))
      {
        Parameter::reset(p);
        value_=op->value_;
      }
      else
        throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
    }

};




#define declareDimensionedParameter(baseType, baseTypeName, dimensionType, dimensionTypeName) \
  extern char baseTypeName##dimensionTypeName[]; \
  typedef SimpleDimensionedParameter<baseType, dimensionType, baseTypeName##dimensionTypeName> baseTypeName##dimensionTypeName##Parameter



#define declareDimensionedParameters(baseType, baseTypeName) \
  declareDimensionedParameter(baseType, baseTypeName, boost::units::si::length, Length); \
  declareDimensionedParameter(baseType, baseTypeName, boost::units::si::velocity, Velocity)



declareDimensionedParameters(double, scalar);
declareDimensionedParameters(arma::mat, vector);



} // namespace insight

#endif // INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H
