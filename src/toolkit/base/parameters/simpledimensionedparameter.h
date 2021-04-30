#ifndef INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H
#define INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H


#include "base/parameter.h"
#include "base/units.h"


namespace insight {



template<class T, class Unit, char const* N>
class SimpleDimensionedParameter
    : public Parameter
{

public:
    typedef T base_value_type;
    typedef Unit unit_type;
    typedef boost::units::quantity<unit_type, base_value_type> value_type;

protected:
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
          value_ ( value )
    {}

    SimpleDimensionedParameter
    (
        const base_value_type& value,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
    )
        : Parameter ( description, isHidden, isExpert, isNecessary, order ),
          value_ ( value*unit_type() )
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
      value_ = newValue * unit_type();
    }

    base_value_type getInDefaultUnit() const
    {
      return value_.value();
    }

    std::string latexRepresentation() const override
    {
        return SimpleLatex( valueToString ( value_.value() ) + boost::units::symbol_string(unit_type()) ).toLaTeX();
    }

    std::string plainTextRepresentation(int /*indent*/=0) const override
    {
        return SimpleLatex( valueToString ( value_.value() ) + boost::units::symbol_string(unit_type()) ).toPlainText();
    }


    Parameter* clone() const override
    {
      using namespace boost::units;
        return new SimpleDimensionedParameter<T, Unit, N>
            (
              value_.value(),
              description_.simpleLatex(),
              isHidden_, isExpert_, isNecessary_, order_
            );
    }

    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override
    {
        using namespace rapidxml;
        xml_node<>* child = Parameter::appendToNode ( name, doc, node, inputfilepath );
        base_value_type nv = value_.value();
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
          value_ = value_type(nv * Unit());
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
      if (const auto* op = dynamic_cast<const SimpleDimensionedParameter<T,Unit,N>*>(&p))
      {
        Parameter::reset(p);
        value_=op->value_;
      }
      else
        throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
    }

};




#define declareDimensionedParameter(baseType, baseTypeName, _unit, dimensionTypeName) \
  extern char baseTypeName##dimensionTypeName[]; \
  typedef SimpleDimensionedParameter<baseType, _unit, baseTypeName##dimensionTypeName> \
      baseTypeName##dimensionTypeName##Parameter



#define declareDimensionedParameters(baseType, baseTypeName) \
  declareDimensionedParameter(baseType, baseTypeName, boost::units::si::length, Length); \
  declareDimensionedParameter(baseType, baseTypeName, boost::units::si::velocity, Velocity)



#define defineDimensionedParameter(baseType, baseTypeName, dimensionTypeName) \
  char baseTypeName##dimensionTypeName[] = #baseTypeName#dimensionTypeName; \
  template<> defineType(baseTypeName##dimensionTypeName##Parameter);\
  addToFactoryTable(Parameter, baseTypeName##dimensionTypeName##Parameter)



#define defineDimensionedParameters(baseType, baseTypeName) \
  defineDimensionedParameter(baseType, baseTypeName, Length); \
  defineDimensionedParameter(baseType, baseTypeName, Velocity)




declareDimensionedParameters(double, scalar);
//declareDimensionedParameters(arma::mat, vector);



} // namespace insight

#endif // INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H
