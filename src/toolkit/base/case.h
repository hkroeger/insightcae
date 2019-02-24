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
//      if (elem.isUnique())
//      {
//        bool conflict=false;
//        std::string conflict_ce_name;

//        for (const CaseElement& e: elements_)
//        {
//          if (elem.inConflict(e))
//          {
//            conflict=true;
//            conflict_ce_name=e.name();
//          }
//        }

//        if (conflict)
//        {
//          throw insight::Exception(
//                "Could not insert CE "+elem.name()+":"
//                " conflict with "+conflict_ce_name+"!"
//                );
//        }

//      }
      elements_.push_back(elem);
      return static_cast<T>(&elements_.back());
    }

    template<class T>
    T* get(const std::string& name)
    {
      boost::regex re_name(name);
      T* res=NULL;
      for (CaseElement& el: elements_)
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
      for (const CaseElement& el: elements_)
      {
        if ( boost::regex_match(el.name(), re_name) )
        {
        res=dynamic_cast<const T*>(&el);
        if (res) break;
        }
      }
      return res;
    }
    
    template<class T>
    void remove(const std::string& name)
    {
      boost::regex re_name(name);
      T* res=NULL;
      for(boost::ptr_vector<CaseElement>::iterator i=elements_.begin(); i!=elements_.end(); i++)
      {
        if ( boost::regex_match((*i).name(), re_name) )
        {
            res=dynamic_cast<T*>(&(*i));
            if (res) 
            {
                elements_.erase(i);
                return;
            }
        }
      }
    }


    template<class T>
    std::set<T *> findElements()
    {
        std::set<T *> es;
        for (auto& i: elements_)
        {
            if (T *e = dynamic_cast<T*>( &i ))
            {
                es.insert(e);
            }
        }
        return es;
    }

    template<class T>
    T& getUniqueElement()
    {
        std::set<T*> es = const_cast<Case*>(this)->findElements<T>();
        if (es.size()>1)
        {
            throw insight::Exception ( "Case::getUniqueElement(): Multiple elements of requested type "+T::typeName+" in queried case!" );
        }
        if (es.size()==0) {
            throw insight::Exception ( "Case::getUniqueElement(): No element of requested type "+T::typeName+" in queried case found !" );
        }
        return **(es.begin());
    }

    template<class T>
    const T& findUniqueElement() const
    {
        return const_cast<const T&> ( const_cast<Case*>(this)->getUniqueElement<T>() );
    }

    inline const ParameterSet& params() const
    { return parameters_; }
    
    virtual void createOnDisk
    (
        const boost::filesystem::path& location, 
        const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles = std::shared_ptr<std::vector<boost::filesystem::path> >()
    ) =0;
    
//    virtual void run() =0;
};

#ifdef SWIG
%template(insertCaseElement) insight::Case::insert<CaseElement*>;
#endif

}

#endif // CASE_H
