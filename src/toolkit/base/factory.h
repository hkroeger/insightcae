/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INSIGHT_FACTORY_H
#define INSIGHT_FACTORY_H

#include "boost/ptr_container/ptr_map.hpp"

namespace insight {
  
struct NoParameters
{
};

template<class baseT, class paramS>
class Factory
{

public:
    virtual baseT* operator()(const paramS& p) const =0;
};

template<class baseT, class specT, class paramS>
class SpecFactory
: public Factory<baseT, paramS>
{
public:
  
  virtual baseT* operator()(const paramS& p) const
  {
    return new specT(p);
  }
};

#define declareType(typenameStr) \
 static const char *typeName_() { return typenameStr; }; \
 static const std::string typeName; \
 virtual const std::string& type() const { return typeName; } 
 
#define defineType(T) \
 const std::string T::typeName( T::typeName_() )

#define declareFactoryTable(baseT, paramS) \
 typedef boost::ptr_map<std::string, insight::Factory<baseT, paramS> > FactoryTable; \
 static FactoryTable factories_; \
 static baseT* lookup(const std::string& key, const paramS& cp) \

#define defineFactoryTable(baseT, paramS) \
 boost::ptr_map<std::string, insight::Factory<baseT, paramS> > baseT::factories_; \
 baseT* baseT::lookup(const std::string& key, const paramS& cp) \
 { \
   baseT::FactoryTable::const_iterator i = baseT::factories_.find(key); \
  if (i==baseT::factories_.end()) \
    throw insight::Exception("Could not lookup type "+key+" in factory table of type " +#baseT); \
  return (*i->second)( cp ); \
 }


#define addToFactoryTable(baseT, specT, paramS) \
static struct add##specT##To##baseT##FactoryTable \
{\
  add##specT##To##baseT##FactoryTable()\
  {\
    std::string key(specT::typeName); \
    /*std::cout << "Adding entry " << key << " to " #baseT "FactoryTable" << std::endl;*/ \
    baseT::factories_.insert(key, new insight::SpecFactory<baseT, specT, paramS>() ); \
  }\
  ~add##specT##To##baseT##FactoryTable()\
  {\
    std::string key(specT::typeName); \
    /*std::cout << "Removing entry " << key << " from " #baseT "FactoryTable" << std::endl;*/ \
    baseT::factories_.erase(baseT::factories_.find(key)); \
  }\
} v_add##specT##To##baseT##FactoryTable;

}

#endif // INSIGHT_FACTORY_H
