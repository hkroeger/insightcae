/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H
#define INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H


#include "base/tools.h"
#include "base/parameter.h"
#include "base/units.h"
#include "base/rapidxml.h"

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

    bool isDifferent(const Parameter& p) const override
    {
        if (const auto *sp = dynamic_cast<const SimpleDimensionedParameter<T,Unit,N>*>(&p))
        {
            return
                (value_!=(*sp)())
                ;
        }
        else
            return true;
    }

    virtual void set(const value_type& nv)
    {
        value_=nv;
        triggerValueChanged();
    }

    virtual const value_type& operator() () const
    {
        return value_;
    }

    void setInDefaultUnit(const base_value_type& newValue)
    {
        set(newValue * unit_type());
    }

    base_value_type getInDefaultUnit() const
    {
      return value_.value();
    }

    std::string latexRepresentation(
        const std::string&,
        int,
        const FileStorageInfo& ) const override
    {
        return SimpleLatex( toString ( value_.value() )
                           + boost::units::symbol_string(unit_type()) ).toLaTeX();
    }

    std::string plainTextRepresentation(int /*indent*/) const override
    {
        return SimpleLatex( toString ( value_.value() )
                           + boost::units::symbol_string(unit_type()) ).toPlainText();
    }


    std::unique_ptr<Element> clone() const override
    {
        using namespace boost::units;
        auto p = std::make_unique<SimpleDimensionedParameter<T, Unit, N> >
            (
              value_.value(),
              description().simpleLatex(),
              isHidden(), isExpert(), isNecessary(), order()
            );
        return p;
    }

    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override
    {
        using namespace rapidxml;

        xml_node<>* child = Parameter::appendToNode (
            name, doc, node, outProps );

        appendAttribute(doc, *child, "value", value_.value() );

        return child;
    }

    const rapidxml::xml_node<>* readFromNode
    (
        const std::string& name,
        const rapidxml::xml_node<>& node
    ) override
    {
        using namespace rapidxml;
      using namespace boost::units;
        auto* child = Parameter::readFromNode ( name, node );
        if ( child )
        {
          value_ = value_type(getMandatoryAttribute<base_value_type>(*child, "value") * Unit());
          triggerValueChanged();
        }
        else
        {
          insight::Warning(
                boost::str(
                  boost::format(
                   "No xml node found with type '%s' and name '%s', default value '%s' is used."
                   ) % type() % name % plainTextRepresentation(0)
                 )
              );
        }
        return child;
    }


    SimpleDimensionedParameter(const rapidxml::xml_node<> & node)
        : Parameter(node)
    {
#warning need to restore unit
        value_ = value_type(getMandatoryAttribute<base_value_type>(node, "value") * Unit());
    }


    void assignFrom(const Element& e)
    {
        auto&op=dynamic_cast<const SimpleDimensionedParameter&>(e);

        value_=op.value_;

        Parameter::assignFrom(op);
    }

    bool isEqual(const Element& op) const  override
    {
        if (auto *oa = dynamic_cast<const SimpleDimensionedParameter*>(&op))
        {
            return (value_==oa->value_);
        }
        else
            return false;
    }

    int nChildren() const override
    {
      return 0;
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
  addParameterFactories(baseTypeName##dimensionTypeName##Parameter)



#define defineDimensionedParameters(baseType, baseTypeName) \
  defineDimensionedParameter(baseType, baseTypeName, Length); \
  defineDimensionedParameter(baseType, baseTypeName, Velocity)




declareDimensionedParameters(double, scalar);
//declareDimensionedParameters(arma::mat, vector);



} // namespace insight

#endif // INSIGHT_SIMPLEDIMENSIONEDPARAMETER_H
