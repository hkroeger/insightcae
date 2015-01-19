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


#ifndef INSIGHT_PARAMETERSET_H
#define INSIGHT_PARAMETERSET_H

#include "base/exception.h"
#include "base/parameter.h"

#include "boost/ptr_container/ptr_map.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/algorithm/string.hpp"

#include "rapidxml/rapidxml.hpp"

#include <map>
#include <vector>
#include <iostream>

namespace insight {
  
class SubsetParameter;

class ParameterSet
: public boost::ptr_map<std::string, Parameter>
{
  
public:
  typedef boost::shared_ptr<ParameterSet> Ptr;
  typedef boost::tuple<std::string, Parameter*> SingleEntry;
  typedef std::vector< boost::tuple<std::string, Parameter*> > EntryList;

public:
    ParameterSet();
    ParameterSet(const ParameterSet& o);
    ParameterSet(const EntryList& entries);
    virtual ~ParameterSet();

    EntryList entries() const;
    void extend(const EntryList& entries);
    
    template<class T>
    T& get(const std::string& name)
    {
      using namespace boost;
      using namespace boost::algorithm;
      
      if (boost::contains(name, "/"))
      {
	std::string prefix = copy_range<std::string>( *make_split_iterator(name, first_finder("/")) );
	std::string remain=name;
	erase_head(remain, prefix.size()+1);
	//std::cout<<prefix<<" >> "<<remain<<std::endl;
	return this->getSubset(prefix).get<T>(remain);
      }
      else
      {
	iterator i = find(name);
	if (i==end())
	{
	  throw insight::Exception("Parameter "+name+" not found in parameterset");
	}
	else
	{
	  typedef T PT;
	  PT* const pt=dynamic_cast<PT* const>(i->second);
	  if (!pt)
	    throw insight::Exception("Parameter "+name+" not of requested type!");
	  else
	    return (*pt);
	}
      }
    }
    
    template<class T>
    const T& get(const std::string& name) const
    {
      using namespace boost;
      using namespace boost::algorithm;
      
      if (boost::contains(name, "/"))
      {
	std::string prefix = copy_range<std::string>( *make_split_iterator(name, first_finder("/")) );
	std::string remain=name;
	erase_head(remain, prefix.size()+1);
	//std::cout<<prefix<<" >> "<<remain<<std::endl;
	return this->getSubset(prefix).get<T>(remain);
      }
      else
      {
	const_iterator i = find(name);
	if (i==end())
	{
	  throw insight::Exception("Parameter "+name+" not found in parameterset");
	}
	else
	{
	  typedef T PT;
	  const PT* const pt=dynamic_cast<const PT* const>(i->second);
	  if (!pt)
	    throw insight::Exception("Parameter "+name+" not of requested type!");
	  else
	    return (*pt);
	}
      }
     }
     
    template<class T>
    const typename T::value_type& getOrDefault(const std::string& name, const typename T::value_type& defaultValue) const
    {
      try 
      {
	return this->get<T>(name)();
      }
      catch (insight::Exception e)
      {
	return defaultValue;
      }
    }
    
    inline bool contains(const std::string& name) const
    {
      const_iterator i = find(name);
      return (i!=end());
    }

    inline int& getInt(const std::string& name) { return this->get<IntParameter>(name)(); }
    inline double& getDouble(const std::string& name) { return this->get<DoubleParameter>(name)(); }
    inline bool& getBool(const std::string& name) { return this->get<BoolParameter>(name)(); }
    inline std::string& getString(const std::string& name) { return this->get<StringParameter>(name)(); }
    inline arma::mat& getVector(const std::string& name) { return this->get<VectorParameter>(name)(); }
    inline boost::filesystem::path& getPath(const std::string& name) 
    { return this->get<PathParameter>(name)(); }
    ParameterSet& getSubset(const std::string& name);

    inline const int& getInt(const std::string& name) const { return this->get<IntParameter>(name)(); }    
    inline const double& getDouble(const std::string& name) const { return this->get<DoubleParameter>(name)(); }
    inline const bool& getBool(const std::string& name) const { return this->get<BoolParameter>(name)(); }
    inline const std::string& getString(const std::string& name) const { return this->get<StringParameter>(name)(); }
    inline const arma::mat& getVector(const std::string& name) const { return this->get<VectorParameter>(name)(); }
    inline const boost::filesystem::path& getPath(const std::string& name) const
    { return this->get<PathParameter>(name)(); }
    const ParameterSet& getSubset(const std::string& name) const;
    inline const ParameterSet& operator[](const std::string& name) const { return getSubset(name); }
    
    inline void replace(const std::string& key, Parameter* newp)
    {
      using namespace boost;
      using namespace boost::algorithm;
      
      if (boost::contains(key, "/"))
      {
	std::string prefix = copy_range<std::string>( *make_split_iterator(key, first_finder("/")) );
	std::string remain = key;
	erase_head(remain, prefix.size()+1);
	std::cout<<prefix<<" >> "<<remain<<std::endl;
	return this->getSubset(prefix).replace(remain, newp);
      }
      else
      {
	boost::ptr_map<std::string, Parameter>::replace(this->find(key), newp);
      }
    }
    
    virtual std::string latexRepresentation() const;

    virtual ParameterSet* cloneParameterSet() const;

    virtual void appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
    virtual void readFromNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
    
    virtual void saveToFile(const boost::filesystem::path& file, std::string analysisType = std::string() ) const;
    virtual std::string readFromFile(const boost::filesystem::path& file);

};

typedef boost::shared_ptr<ParameterSet> ParameterSetPtr;

#define PSINT(p, subdict, key) int key = p[subdict].getInt(#key);
#define PSDBL(p, subdict, key) double key = p[subdict].getDouble(#key);
#define PSSTR(p, subdict, key) std::string key = p[subdict].getString(#key);
#define PSBOOL(p, subdict, key) bool key = p[subdict].getBool(#key);
#define PSPATH(p, subdict, key) boost::filesystem::path key = p[subdict].getPath(#key);


class SubsetParameter
: public Parameter,
  public ParameterSet //boost::shared_ptr<ParameterSet>
{
public:
  typedef boost::shared_ptr<SubsetParameter> Ptr;
  typedef ParameterSet value_type;
  
// protected:
//   boost::shared_ptr<ParameterSet> value_;
  
public:
  declareType("subset");
  
  SubsetParameter();
  SubsetParameter(const std::string& description);
  SubsetParameter(const ParameterSet& defaultValue, const std::string& description);
  
  inline void setParameterSet(const ParameterSet& paramset) 
  { 
//     /*value_.*/this->reset(paramset.clone()); 
    this->setParameterSet(paramset);
  }
  void merge(const SubsetParameter& other);
  inline ParameterSet& operator()() { return /**value_*/ static_cast<ParameterSet&>(*this); }
  inline const ParameterSet& operator()() const { return /**value_*/static_cast<const ParameterSet&>(*this); }
  
  virtual std::string latexRepresentation() const;
  
  virtual Parameter* clone () const;

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
};

class SelectableSubsetParameter
: public Parameter
{
public:
  typedef std::string key_type;
  typedef boost::ptr_map<key_type, ParameterSet> ItemList;
  typedef ItemList value_type;
  
  typedef boost::tuple<key_type, ParameterSet*> SingleSubset;
  typedef std::vector< boost::tuple<key_type, ParameterSet*> > SubsetList;
  
protected:
  key_type selection_;
  ItemList value_;
  
public:
  declareType("selectableSubset");
  
  SelectableSubsetParameter(const std::string& description);
  /**
   * Construct from components:
   * \param defaultSelection The key of the subset which is selected per default
   * \param defaultValue A map of key-subset pairs. Between these can be selected
   * \param description The description of the selection parameter
   */
  SelectableSubsetParameter(const key_type& defaultSelection, const SubsetList& defaultValue, const std::string& description);
  
  inline key_type& selection() { return selection_; }
  inline const key_type& selection() const { return selection_; }
  inline const ItemList& items() { return value_; }
  inline ParameterSet& operator()() { return *(value_.find(selection_)->second); }
  inline const ParameterSet& operator()() const { return *(value_.find(selection_)->second); }

  virtual std::string latexRepresentation() const;

  virtual Parameter* clone () const;

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath);
};


}


#endif // INSIGHT_PARAMETERSET_H
