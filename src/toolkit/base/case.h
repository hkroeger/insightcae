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


#ifndef CASE_H
#define CASE_H

#include "base/parameterset.h"
#include "base/caseelement.h"

#include "boost/ptr_container/ptr_vector.hpp"

namespace insight
{

struct TemporaryCaseDir
{
  bool keep_;
  boost::filesystem::path dir;
  
  TemporaryCaseDir(bool keep=false, const std::string& prefix="");
  ~TemporaryCaseDir();
};
  


class Case
{
  
protected:
  ParameterSet parameters_;
  boost::ptr_vector<CaseElement> elements_;

public:
    Case();
    Case(const Case& other);
    virtual ~Case();
    
    //CaseElement const* insert(CaseElement* elem);
    template<class T>
    T insert(T elem)
    {
      elements_.push_back(elem);
      return static_cast<T>(&elements_.back());
    }

    template<class T>
    T* get(const std::string& name)
    {
      boost::regex re_name(name);
      T* res=NULL;
      BOOST_FOREACH(CaseElement& el, elements_)
      {
	if ( boost::regex_match(el.name(), re_name) )
	{
	  res=dynamic_cast<T*>(&el);
	  if (res) break;
	}
      }
      return res;
    }
    
    template<class T>
    const T* get(const std::string& name) const
    {
      boost::regex re_name(name);
      const T* res=NULL;
      BOOST_FOREACH(const CaseElement& el, elements_)
      {
	if ( boost::regex_match(el.name(), re_name) )
	{
	  res=dynamic_cast<const T*>(&el);
	  if (res) break;
	}
      }
      return res;
    }
    
    inline const ParameterSet& params() const
    { return parameters_; }
    
    virtual void createOnDisk
    (
        const boost::filesystem::path& location, 
        const boost::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles = boost::shared_ptr<std::vector<boost::filesystem::path> >()
    ) =0;
    
//    virtual void run() =0;
};

#ifdef SWIG
%template(insertCaseElement) insight::Case::insert<CaseElement*>;
#endif

}

#endif // CASE_H
