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




class Parameter
    : public boost::noncopyable
{

public:
    declareFactoryTable ( Parameter, LIST ( const std::string& descr ), LIST ( descr ) );

protected:
    SimpleLatex description_;

    bool isHidden_, isExpert_, isNecessary_;
    int order_;

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

    virtual void reset(const Parameter&);



    /**
     * @brief The SearchResultParentInDict struct
     * contains the result of the parent search operation
     * for the case that the parameter was contained in a dictionary
     */
    struct SearchResultParentInDict {
      /**
      * @brief myIterator
      * iterator to the current parameter in the parent parameter set
      */
     std::map<std::string, std::unique_ptr<Parameter> >::const_iterator myIterator;

     /**
      * @brief myParentSet
      * pointer to the containing (parent) parameter set
      */
     const ParameterSet* myParentSet = nullptr;

     /**
      * @brief myParentSetParameter
      * if the containing parameter set is wrapped in a parameter, pointer to the
      * parameter. Otherwise (if contained in the top level) a nullptr
      */
     const Parameter* myParentSetParameter = nullptr;
    };


    /**
     * @brief The SearchResultParentInArray struct
     * contains the result of the parent search operation
     * for the case that the parameter was contained in an array
     */
    struct SearchResultParentInArray {
      int i = -1;
      const Parameter* myParentArrayParameter = nullptr;
    };


    typedef boost::variant<SearchResultParentInDict,SearchResultParentInArray,boost::blank> SearchParentResult;

    /**
     * @brief searchMyParentIn
     * searches the parent section of this parameter in the given parameter set.
     * An exception is thrown, if the parameter was not found in the set.
     * @param ps
     * ParameterSet to be searched.
     * @return
     * SearchParentResult struct
     */
    SearchParentResult searchMyParentIn(const ParameterSet& ps) const;
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
