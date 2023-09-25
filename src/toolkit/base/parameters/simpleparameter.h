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

namespace insight {



template<class T, char const* N>
class SimpleParameter
    : public Parameter
{

public:
    typedef T value_type;

protected:
    T value_;

public:
    declareType ( N );

    SimpleParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 )
        : Parameter ( description, isHidden, isExpert, isNecessary, order )
    {}

    SimpleParameter ( const T& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 )
        : Parameter ( description, isHidden, isExpert, isNecessary, order ),
          value_ ( value )
    {}

    bool isDifferent(const Parameter& p) const override
    {
      if (const auto *sp = dynamic_cast<const SimpleParameter<T,N>*>(&p))
      {
        return (value_!=(*sp)());
      }
      else
        return true;
    }

    virtual T& operator() ()
    {
        return value_;
    }
    virtual const T& operator() () const
    {
        return value_;
    }

    std::string latexRepresentation() const override
    {
        return SimpleLatex( valueToString ( value_ ) ).toLaTeX();
    }

    std::string plainTextRepresentation(int /*indent*/=0) const override
    {
        return SimpleLatex( valueToString ( value_ ) ).toPlainText();
    }


    Parameter* clone() const override
    {
        return new SimpleParameter<T, N> ( value_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_ );
    }

    rapidxml::xml_node<>* appendToNode (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override
    {
        insight::CurrentExceptionContext ex("appending simple parameter "+name+" to node "+node.name());

        using namespace rapidxml;
        xml_node<>* child = Parameter::appendToNode ( name, doc, node, inputfilepath );
        child->append_attribute
        (
            doc.allocate_attribute
            (
                "value",
                doc.allocate_string ( valueToString ( value_ ).c_str() )
            )
        );
        return child;
    }

    void readFromNode
    (
        const std::string& name,
        rapidxml::xml_node<>& node,
        boost::filesystem::path
    ) override
    {
        using namespace rapidxml;
        xml_node<>* child = findNode ( node, name, type() );
        if ( child )
        {
          auto valueattr=child->first_attribute ( "value" );
          insight::assertion(valueattr, "No value attribute present in "+name+"!");
          stringToValue ( valueattr->value(), value_ );
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
      if (const auto* op = dynamic_cast<const SimpleParameter<T,N>*>(&p))
      {
        Parameter::reset(p);
        value_=op->value_;
      }
      else
        throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
    }

};




extern char DoubleName[];
extern char IntName[];
extern char BoolName[];
extern char VectorName[];
extern char StringName[];

typedef SimpleParameter<double, DoubleName> DoubleParameter;
typedef SimpleParameter<int, IntName> IntParameter;
typedef SimpleParameter<bool, BoolName> BoolParameter;
typedef SimpleParameter<arma::mat, VectorName> VectorParameter;
typedef SimpleParameter<std::string, StringName> StringParameter;


#ifdef SWIG
%template(DoubleParameter) SimpleParameter<double, DoubleName>;
%template(IntParameter) SimpleParameter<int, IntName>;
%template(BoolParameter) SimpleParameter<bool, BoolName>;
%template(VectorParameter) SimpleParameter<arma::mat, VectorName>;
%template(StringParameter) SimpleParameter<std::string, StringName>;
#endif


}

#endif // SIMPLEPARAMETER_H
