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

#include "factory.h"
#include "base/latextools.h"
#include "base/linearalgebra.h"
#include "base/exception.h"

#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include <set>
#include <sstream>

#include "base/boost_include.h"
#include "boost/signals2.hpp"

#include <openssl/md5.h>

#include "rapidxml/rapidxml.hpp"







namespace insight {


template<class T, typename ...Args>
std::unique_ptr<T> make(Args... args)
{
        return std::unique_ptr<T>(new T(args...));
}

  



rapidxml::xml_node<> *findNode(rapidxml::xml_node<>& father, const std::string& name, const std::string& typeName);


void writeMatToXMLNode(const arma::mat& matrix, rapidxml::xml_document< char >& doc, rapidxml::xml_node< char >& node);


class Parameter;
class ParameterSet;



class ArrayParameterBase
{
public:
  virtual ~ArrayParameterBase();

  virtual int size() const =0;
  Parameter& elementRef ( int i );
  virtual const Parameter& element( int i ) const =0;
};



//#ifndef SWIG

//template<typename Signature>
//class Signal
//{
//public:
//  typedef
//      std::shared_ptr<boost::signals2::scoped_connection>
//          ScopedConnectionPtr;
//  typedef
//      std::weak_ptr<boost::signals2::scoped_connection>
//          WeakScopedConnectionPtr;

//  typedef boost::signals2::signal<Signature> base_signal;

//private:
//  base_signal signal_;
//  std::vector<WeakScopedConnectionPtr> connections_;

//public:
//  ScopedConnectionPtr connect(const typename base_signal::slot_type& slot);

//  template<typename ...Args>
//  typename base_signal::result_type operator()(Args&&... addArgs)
//  {
//    return signal_(std::forward<Args>(addArgs)...);
//  }
//};


//#endif




class Parameter
    : public boost::noncopyable
{

public:
    declareFactoryTable ( Parameter, LIST ( const std::string& descr ), LIST ( descr ) );

#ifndef SWIG
    boost::signals2::signal<void()> valueChanged, childValueChanged;
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
        std::string name() const;
    };

    typedef std::reverse_iterator<iterator> reverse_iterator; //optional
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator; //optional

protected:
    SimpleLatex description_;

    bool isHidden_, isExpert_, isNecessary_;
    int order_;

    bool valueChangeSignalBlocked_;

public:
    declareType ( "Parameter" );

    Parameter();
    Parameter ( const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order);
    virtual ~Parameter();

    bool isHidden() const;
    bool isExpert() const;
    bool isNecessary() const;
    virtual bool isDifferent(const Parameter& p) const;
    int order() const;

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


    virtual void saveToStream(std::ostream& os, const boost::filesystem::path& parentPath, std::string analysisName = std::string() ) const;
    void saveToFile ( const boost::filesystem::path& file, std::string analysisType = std::string() ) const;
    void saveToString ( std::string& s, const boost::filesystem::path& file, std::string analysisType = std::string() ) const;
    std::string readFromFile(const boost::filesystem::path& file, const std::string& startAtSubnode = std::string() );

//    rapidxml::xml_node<> *findNode ( rapidxml::xml_node<>& father, const std::string& name );
    virtual Parameter* clone() const =0;

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
    virtual std::string childParameterName( int i ) const;
    virtual Parameter& childParameterRef ( int i );
    virtual const Parameter& childParameter( int i ) const;

    virtual int childParameterIndex( const std::string& name ) const;
    Parameter& childParameterByNameRef ( const std::string& name );
    const Parameter& childParameterByName ( const std::string& name ) const;


    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator cend() const;



    virtual void setUpdateValueSignalBlockage(bool block=true);
    void triggerValueChanged();
    void triggerChildValueChanged();

};




typedef std::shared_ptr<Parameter> ParameterPtr;




template<class V>
std::string valueToString(const V& value)
{
  return boost::lexical_cast<std::string>(value);
}



std::string valueToString(const arma::mat& value);




template<class V>
void stringToValue(const std::string& s, V& v)
{
  v=boost::lexical_cast<V>(s);
}



void stringToValue(const std::string& s, arma::mat& v);
  


inline Parameter* new_clone(const Parameter& p)
{
  return p.clone();
}




}

#endif // INSIGHT_PARAMETER_H
