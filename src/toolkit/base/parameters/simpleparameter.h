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

#ifndef SIMPLEPARAMETER_H
#define SIMPLEPARAMETER_H

#include "base/parameter.h"
#include "base/rapidxml.h"

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "base/warningdispatcher.h"

namespace insight {

#define declareTemplateType() \
static const char* typeName_(); \
    virtual const std::string& type() const { return typeName; } \
    static const std::string typeName

#define defineTemplateInstanceType(T, typenameStr) \
 template<> const char* T::typeName_() { return typenameStr; } \
 template<> const std::string T::typeName( T::typeName_() )


template<class T>
class SimpleParameter
    : public Parameter
{

public:
    typedef T value_type;

protected:
    T value_;

public:
    declareTemplateType();

    SimpleParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );


    SimpleParameter ( const T& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );


    bool isDifferent(const Parameter& p) const override
    {
      if (const auto *sp = dynamic_cast<const SimpleParameter<T>*>(&p))
      {
        return (value_!=(*sp)());
      }
      else
        return true;
    }


    void set(
        const  T& nv,
        bool skipNotification=false // used in ConstrainedSketchEditor
        )
    {
      value_=nv;
      if (!skipNotification)
      {
        triggerValueChanged();
      }
    }


    virtual const T& operator() () const
    {
        return value_;
    }


    std::string latexRepresentation(
        const std::string&,
        int,
        const FileStorageInfo& ) const override
    {
        return SimpleLatex( toString ( value_ ) ).toLaTeX();
    }

    bool canSetDataFromString() const override
    {
        return true;
    }

    void setDataFromString(const std::string& newValue, bool* ok = nullptr) override
    {
        try
        {
            set(toValue<T>(newValue));
            if (ok) *ok=true;
        }
        catch (...)
        {
            if (ok) *ok=false;
        }
    }

    std::string plainTextRepresentation(int /*indent*/) const override
    {
        return SimpleLatex( toString ( value_ ) ).toPlainText();
    }


protected:
    std::unique_ptr<insight::hierarchicalData::Element> doCloneUninitialized() const override
    {
        auto p= std::make_unique<SimpleParameter<T> >(
            value_,
            description().simpleLatex(),
            isHidden(), isExpert(), isNecessary(), order() );
        return p;
    }

public:
    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override
    {
        insight::CurrentExceptionContext ex(
            insight::VerbosityLevel::Loops,
            "appending simple parameter %s to node %s", name.c_str(), node.name());

        using namespace rapidxml;
        xml_node<>* child = Parameter::appendToNode ( name, doc, node, outProps );
        appendAttribute(doc, *child, "value", value_ );
        return child;
    }


    const rapidxml::xml_node<>* readFromNode
    (
        const std::string &name,
        const rapidxml::xml_node<> &node
    ) override
    {
        auto *child = Parameter::readFromNode( name, node );
        if ( child )
        {
          value_=getMandatoryAttribute<T>(*child, "value");
          triggerValueChanged();
        }
        else
        {
          insight::Warning(
                boost::str(
                  boost::format(
                   "No xml node found with type '%s' and name '%s',"
                        " default value '%s' is used."
                        " Available nodes: %s"
                   )
                    % type() % name
                    % plainTextRepresentation(0)
                    % valueList_to_string(listNodes(node), 99)
                 )
              );
        }
        return child;
    }


    SimpleParameter(const rapidxml::xml_node<> & node, bool skipValueRead=false)
        : Parameter(node)
    {
        if (!skipValueRead)
            value_=getMandatoryAttribute<T>(node, "value");
    }


    void assignFrom(const Element& e) override
    {
        auto& op = dynamic_cast<const SimpleParameter&>(e);

        value_=op.value_;

        Parameter::assignFrom(op);
    }

    bool isEqual(const insight::hierarchicalData::Element &op) const override
    {
        if (auto *oa = dynamic_cast<const SimpleParameter*>(&op))
        {
            return value_==oa->value_;
        }
        else
            return false;
    }

    int nChildren() const override
    {
      return 0;
    }

    bool isBooleanData() const override
    {
        return Element::isBooleanData();
    }

    bool canSetFromBoolean() const override
    {
        return Element::canSetFromBoolean();
    }

    bool getAsBoolean() const override
    {
        return Element::getAsBoolean();
    }

    void setBoolean(bool b) override
    {
        Element::setBoolean(b);
    }

};



template<class T>
SimpleParameter<T>::SimpleParameter (
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
    : Parameter ( description, isHidden, isExpert, isNecessary, order )
{}

template<class T>
SimpleParameter<T>::SimpleParameter (
    const T& value,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
    : Parameter ( description, isHidden, isExpert, isNecessary, order ),
    value_ ( value )
{}

typedef SimpleParameter<double> DoubleParameter;
typedef SimpleParameter<int> IntParameter;
typedef SimpleParameter<bool> BoolParameter;
typedef SimpleParameter<std::string> StringParameter;
typedef SimpleParameter<boost::gregorian::date> DateParameter;
typedef SimpleParameter<boost::posix_time::ptime> DateTimeParameter;



template<>
bool BoolParameter::isBooleanData() const;

template<>
bool BoolParameter::canSetFromBoolean() const;

template<>
bool BoolParameter::getAsBoolean() const;

template<>
void BoolParameter::setBoolean(bool b);

}


#ifdef SWIG
%template(DoubleParameter) insight::SimpleParameter<double>;
%template(IntParameter) insight::SimpleParameter<int>;
%template(BoolParameter) insight::SimpleParameter<bool>;
%template(StringParameter) insight::SimpleParameter<std::string>;
%template(DateParameter) insight::SimpleParameter<boost::gregorian::date>;
%template(DateTimeParameter) insight::SimpleParameter<boost::posix_time::ptime>;
%template(VectorParameterBase) insight::SimpleParameter<arma::mat>;
#endif




namespace insight
{


class VectorParameter
    : public SimpleParameter<arma::mat>
{
public:
    enum VectorType {
        NonSpatial, Point, Direction
    };

private:
    VectorType vectorType_;

public:
    declareType ( "vector" );

    VectorParameter (
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    VectorParameter (
        const arma::mat& value, const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    VectorParameter (
        VectorType vt, const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    VectorParameter (
        VectorType vt, const arma::mat& value,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override;

    VectorParameter(const rapidxml::xml_node<> & node, bool skipValueRead=false);

    inline VectorType vectorType() const
    {
        return vectorType_;
    }

protected:
    std::unique_ptr<insight::hierarchicalData::Element> doCloneUninitialized() const override;
};



}

#endif // SIMPLEPARAMETER_H
