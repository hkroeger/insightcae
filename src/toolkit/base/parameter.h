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


#ifndef INSIGHT_PARAMETER_H
#define INSIGHT_PARAMETER_H

#include "boost/date_time/posix_time/ptime.hpp"
#include "factory.h"
#include "base/latextools.h"
#include "base/tools.h"
#include "base/hierarchicalelement.h"
#include "base/linearalgebra.h"
#include "base/exception.h"
#include "base/cppextensions.h"
#include "base/parametersbase.h"

#include <memory>
#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include <set>
#include <sstream>
#include <type_traits>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/signals2.hpp>

#include <openssl/md5.h>

#include "rapidxml/rapidxml.hpp"




namespace insight {

namespace cad {
class ConstrainedSketch;
}




/**
 * @brief The PrimitiveStaticValueWrap class
 * Wraps a parameter value along with its path
 * into the parameter set, which contains it.
 */
template<class T>
struct PrimitiveStaticValueWrap
{
    T value;
    boost::optional<std::string> parameterPath;

    PrimitiveStaticValueWrap()
    {}

    PrimitiveStaticValueWrap(const PrimitiveStaticValueWrap& o)
    : value(o.value), parameterPath(o.parameterPath)
    {}

    PrimitiveStaticValueWrap(const T& v)
        : value(v)
    {}

    PrimitiveStaticValueWrap&
    setPath(const std::string &p)
    {
        parameterPath=p;
        return *this;
    }

    operator T&() { return value; }
    operator const T&() const { return value; }
};



/**
 * @brief The StaticValueWrap class
 * Wraps a parameter value along with its path
 * into the parameter set, which contains it.
 */
template<class T>
class StaticValueWrap
    : public T
{

public:
    boost::optional<std::string> parameterPath;

    StaticValueWrap()
    {}

    StaticValueWrap(const StaticValueWrap& o)
    : T(o),
      parameterPath(o.parameterPath)
    {}

    template<class OT>
    StaticValueWrap(const StaticValueWrap<OT>& o)
        : T(static_cast<const T&>(o)),
        parameterPath(o.parameterPath)
    {}

    StaticValueWrap(const T& t)
    : T(t)
    {}

    using T::T;

    StaticValueWrap&
    setPath(const std::string &p)
    {
        if constexpr (std::is_base_of_v<insight::ParametersBase, T>)
        {
            static_cast<insight::ParametersBase&>(*this).parameterPath = p;
        }
        parameterPath=p;
        return *this;
    }

    inline T& wrappedValue() { return *this; }
    inline const T& wrappedValue() const { return *this; }
    // operator const T&() const { return *this; }
};



template<class P>
class ParametersReference
    : public std::reference_wrapper<const P>
{
public:
    boost::optional<std::string> parameterPath;

    ParametersReference(const P& o, const boost::optional<std::string>& explicitPath)
        : std::reference_wrapper<const P>(o),
        parameterPath(explicitPath)
    {}

    ParametersReference(const ParametersReference<P>& o)
        : std::reference_wrapper<const P>(o),
        parameterPath(o.parameterPath)
    {}

    template<class DerivedP>
    ParametersReference(const ParametersReference<DerivedP>& o)
      : std::reference_wrapper<const P>(dynamic_cast<const P&>(o.get())),
        parameterPath(o.parameterPath)
    {}

    ParametersReference(const StaticValueWrap<P>& parameter)
        : std::reference_wrapper<const P>(parameter),
        parameterPath(parameter.parameterPath)
    {}

    template<class DerivedP>
    ParametersReference(const StaticValueWrap<DerivedP>& parameter)
        : std::reference_wrapper<const P>(parameter),
        parameterPath(parameter.parameterPath)
    {}

    const P& parameters() const
    {
        return this->get();
    }
};


}




namespace arma
{


template<>
struct is_Mat< const insight::StaticValueWrap<Mat<double> > >
{ static const bool value = true; };

template<>
class Proxy<insight::StaticValueWrap<Mat<double> > >
    : public Proxy<arma::mat>
{
public:
    inline Proxy(const insight::StaticValueWrap<Mat<double> >& A)
        : Proxy<arma::mat>(static_cast<const arma::mat&>(A))
    {}
};


}


namespace insight
{


class Parameter;
class ParameterSet;



/**
 * @brief The Parameter class
 * - contains the abstract interface for querying child parameters
 *   but does not actually contain them.
 * - can emit signal when its value is changed
 * - another signal when the value of one of its child has changed
 * - stores a reference to its parent parameter,
 *   so has pointer semantics
 */
class Parameter
    : public hierarchicalData::Element
{

public:
    // declareFactoryTable ( Parameter, LIST ( const std::string& descr ), LIST ( descr ) );
    declareFactoryTable2(
        Parameter,
        ParameterFromDescription, createParameter,
        const std::string&
        );

    declareFactoryTable2(
        Parameter,
        ParameterFromNode, createParameterFromNode,
        const rapidxml::xml_node<> & );

#define addParameterFactories(PT) \
    addToFactoryTable2(Parameter, ParameterFromDescription, createParameter, PT);\
    addToFactoryTable2(Parameter, ParameterFromNode, createParameterFromNode, PT);

public:
    void setParent(Element* parent) override;

private:
    SimpleLatex description_;

    bool isHidden_, isExpert_, isNecessary_;


    friend class ArrayParameter;
    friend class ParameterSet;
    friend class SelectableSubsetParameter;
    friend class LabeledArrayParameter;
    friend class cad::ConstrainedSketch;


public:
    declareType ( "Parameter" );

    Parameter( const rapidxml::xml_node<> &node );

    Parameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );


    bool hasParentSet() const;
    ParameterSet& parentSet();
    const ParameterSet& parentSet() const;

    bool isHidden() const;
    bool isExpert() const;
    bool isNecessary() const;
    virtual bool isDifferent(const Parameter& p) const;

    bool isModified(const ParameterSet& defaultValues) const;

    inline const SimpleLatex& description() const
    {
        return description_;
    }
    
    inline SimpleLatex& description()
    {
        return description_;
    }


    /**
     * @brief resolveRelativePaths
     * if paths are stored and they are relative,
     * convert them into absolute ones using this
     * base directory.
     * @param baseDirectory
     */
    virtual void resolveRelativePaths(
        const boost::filesystem::path &baseDirectory);


    /**
     * @brief isPacked
     * check, if contains file contents
     * @return
     */
    virtual bool isPacked() const;


    /**
     * @brief pack
     * pack the external file. Replace stored content, if present.
     */
    virtual void pack();

    /**
     * @brief unpack
     * restore file contents on disk, if file is not there
     */
    virtual void unpack(const boost::filesystem::path& basePath);

    /**
     * @brief clearPackedData
     * remove the file content information
     */
    virtual void clearPackedData();

    void readFromRootNode(
        const rapidxml::xml_node<>& rootNode,
        const std::string& startAtSubnode = std::string() ) override;

    void readFromFile(
        const boost::filesystem::path& file,
        const std::string& startAtSubnode = std::string() ) override;


    rapidxml::xml_node<>* appendToNode(
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const OutputProperties& outProps ) const override;

    /**
     * @brief createFromNode
     * Restore a snapshot from the given node.
     * The resulting parameter data will only be valid for representation
     * and features will be missing. For example, selection parameters won't
     * now other options the the currently selected one.
     * @param node
     * @return
     */
    static std::unique_ptr<Parameter> createFromNode(
        const rapidxml::xml_node<>& node );

    void assignFrom(const Element& o) override;


#ifndef SWIG
    virtual std::unique_ptr<Parameter> intersection(const Parameter& other) const;
#endif

};




}

#endif // INSIGHT_PARAMETER_H
