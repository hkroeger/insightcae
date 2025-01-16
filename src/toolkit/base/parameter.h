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
#include "base/linearalgebra.h"
#include "base/exception.h"
#include "base/cppextensions.h"

#include <memory>
#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include <set>
#include <sstream>
#include <type_traits>

#include "base/boost_include.h"
#include "boost/signals2.hpp"

#include <openssl/md5.h>

#include "rapidxml/rapidxml.hpp"




namespace insight {

namespace cad {
class ConstrainedSketch;
}



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
        parameterPath=p;
        return *this;
    }

    // operator const T&() const { return *this; }
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
    : public boost::noncopyable,
      public std::observable
{

public:
    declareFactoryTable ( Parameter, LIST ( const std::string& descr ), LIST ( descr ) );

#ifndef SWIG
    boost::signals2::signal<void()> valueChanged, childValueChanged;
    boost::signals2::signal<void(int, int)> beforeChildInsertion, childInsertionDone;
    boost::signals2::signal<void(int, int)> beforeChildRemoval, childRemovalDone;
#endif

    typedef Parameter& reference;
    typedef const Parameter& const_reference;
    typedef Parameter* pointer;
    typedef const Parameter* const_pointer;
    typedef size_t size_type;
    typedef int difference_type;

    class const_iterator;

    class iterator
    {
        friend class const_iterator;

        Parameter* p_;
        int iChild_;
    public:
        typedef Parameter::reference reference;
        typedef Parameter::pointer pointer;
        typedef Parameter::size_type size_type;
        typedef Parameter::difference_type difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        iterator();
        iterator(Parameter&, int i=0);
        iterator(const iterator&);
        ~iterator();

        iterator& operator=(const iterator&);
        bool operator==(const iterator&) const;
        bool operator!=(const iterator&) const;

        iterator& operator++();

        reference operator*() const;
        pointer operator->() const;
        pointer get_pointer() const;
        std::string name() const;
    };

    class const_iterator
    {
        const Parameter* p_;
        int iChild_;
    public:
        typedef Parameter::const_reference reference;
        typedef Parameter::const_pointer pointer;
        typedef Parameter::size_type size_type;
        typedef Parameter::difference_type difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        const_iterator();
        const_iterator(const Parameter&, int i=0);
        const_iterator(const iterator&);
        const_iterator(const const_iterator&);
        ~const_iterator();

        const_iterator& operator=(const const_iterator&);
        bool operator==(const const_iterator&) const;
        bool operator!=(const const_iterator&) const;

        const_iterator& operator++();

        reference operator*() const;
        pointer operator->() const;
        pointer get_pointer() const;
        std::string name() const;
    };

    typedef std::reverse_iterator<iterator> reverse_iterator; //optional
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator; //optional

private:
    SimpleLatex description_;

    bool isHidden_, isExpert_, isNecessary_;
    int order_;

    bool valueChangeSignalBlocked_;

    std::observer_ptr<Parameter> parent_;

    bool needsInitialization_;


    friend class ArrayParameter;
    friend class ParameterSet;
    friend class SelectableSubsetParameter;
    friend class LabeledArrayParameter;
    friend class cad::ConstrainedSketch;

protected:
    virtual void setParent(Parameter* parent);

    inline bool valueChangeSignalBlocked() const
    {
        return valueChangeSignalBlocked_;
    }

public:
    declareType ( "Parameter" );

    Parameter (
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    virtual ~Parameter();

    /**
     * @brief initialize
     * required by synchronized parameters (LabeledArrayParameter) that get a selection from other parameters
     */
    virtual void initialize();

    bool hasParent() const;
    Parameter& parent();
    const Parameter& parent() const;
    ParameterSet& parentSet();

    std::string path(bool redirectArrayElementsToDefault=false) const;
    std::string name(bool redirectArrayElementsToDefault=false) const;

    bool isHidden() const;
    bool isExpert() const;
    bool isNecessary() const;
    virtual bool isDifferent(const Parameter& p) const;
    int order() const;

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
     * LaTeX representation of the parameter value
     */
    virtual std::string latexRepresentation() const =0;
    virtual std::string plainTextRepresentation(int indent=0) const =0;

    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
    ) const;

    virtual void readFromNode
    (
        const std::string& name,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
    ) =0;

    void saveToNode(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& rootNode,
        const boost::filesystem::path& parent_path,
        std::string analysisName ) const;
    virtual void saveToStream(std::ostream& os, const boost::filesystem::path& parentPath, std::string analysisName = std::string() ) const;
    void saveToFile ( const boost::filesystem::path& file, std::string analysisType = std::string() ) const;
    void saveToString ( std::string& s, const boost::filesystem::path& file, std::string analysisType = std::string() ) const;

    std::string readFromRootNode(
        rapidxml::xml_node<>& rootNode,
        const boost::filesystem::path& parent_path,
        const std::string& startAtSubnode = std::string() );

    std::string readFromFile(
        const boost::filesystem::path& file,
        const std::string& startAtSubnode = std::string() );

//    rapidxml::xml_node<> *findNode ( rapidxml::xml_node<>& father, const std::string& name );
    virtual std::unique_ptr<Parameter> clone(bool initialize) const =0;

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


    /**
     * @brief copyFrom
     * Set values from other parameter. In subsets, set values of all parameters with the same name as in op
     * @param op
     * other parameter
     */
    virtual void copyFrom(const Parameter& op);

    /**
     * insert entries into current subset, that are not yet present.
     * Existing parameters will not be touched!
     */
    virtual void extend ( const Parameter& op );

    /**
     * Set values and child parmeters from other, overwrite where matching (type and name).
     */
    virtual void merge ( const Parameter& other );

#ifndef SWIG
    virtual std::unique_ptr<Parameter> intersection(const Parameter& other) const;
#endif


    virtual int nChildren() const =0;

    virtual std::string childParameterName(
        int i,
        bool redirectArrayElementsToDefault=false ) const;

    virtual std::string childParameterName(
        const Parameter* childParam,
        bool redirectArrayElementsToDefault=false ) const;

    virtual Parameter& childParameterRef ( int i );

    virtual const Parameter& childParameter( int i ) const;

    virtual int childParameterIndex( const std::string& name ) const;

    virtual int childParameterIndex( const Parameter* childParam ) const;

    Parameter& childParameterByNameRef ( const std::string& name );

    const Parameter& childParameterByName ( const std::string& name ) const;


    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator cend() const;

#ifndef SWIG
    struct UpdateValueSignalBlockage
    {
        Parameter& blockedParameter;
        UpdateValueSignalBlockage(Parameter& p);
        ~UpdateValueSignalBlockage();
    };

    std::unique_ptr<UpdateValueSignalBlockage> blockUpdateValueSignal();
#endif

    virtual void setUpdateValueSignalBlockage(bool block=true);
    void triggerValueChanged();
    void triggerChildValueChanged();
};




template<class V>
std::string valueToString(const V& value)
{
  return boost::lexical_cast<std::string>(value);
}

std::string valueToString(const arma::mat& value);
std::string valueToString(const boost::gregorian::date& value);
std::string valueToString(const boost::posix_time::ptime& value);




template<class V>
void stringToValue(const std::string& s, V& v)
{
  v=boost::lexical_cast<V>(s);
}

void stringToValue(const std::string& s, arma::mat& v);
void stringToValue(const std::string& s, boost::gregorian::date& date);
void stringToValue(const std::string& s, boost::posix_time::ptime& date);


// inline std::unique_ptr<Parameter> new_clone(const Parameter& p)
// {
//   return p.clone();
// }






}

#endif // INSIGHT_PARAMETER_H
