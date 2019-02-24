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


#ifndef INSIGHT_FACTORY_H
#define INSIGHT_FACTORY_H

#include "boost/ptr_container/ptr_map.hpp"

#include "boost/foreach.hpp"


namespace insight {



#define declareType(typenameStr) \
 static const char *typeName_() { return typenameStr; } \
 virtual const std::string& type() const { return typeName; } \
 static const std::string typeName
 
 
 
 
#define defineType(T) \
 const std::string T::typeName( T::typeName_() )


 
#define LIST(...) __VA_ARGS__


 
#define declareFactoryTable(baseT, argList, parList) \
 class Factory\
 {\
 public:\
   virtual ~Factory();\
   virtual baseT* operator()(argList) const =0;\
 };\
 template<class SPEC> \
 class SpecFactory\
 : public Factory \
 {\
 public:\
  virtual ~SpecFactory() {}\
  virtual baseT* operator()(argList) const\
  {\
    return new SPEC(parList);\
  }\
 };\
 typedef std::map<std::string, Factory* > FactoryTable; \
 static FactoryTable* factories_; \
 static baseT* lookup(const std::string& key, argList); \
 static std::vector<std::string> factoryToC()




#define declareFactoryTableNoArgs(baseT) \
 class Factory\
 {\
 public:\
   virtual ~Factory();\
   virtual baseT* operator()() const =0;\
 };\
 template<class SPEC> \
class SpecFactory\
: public Factory \
{\
public:\
  virtual ~SpecFactory() {}\
  virtual baseT* operator()() const\
  {\
    return new SPEC();\
  }\
};\
typedef std::map<std::string, Factory* > FactoryTable; \
static FactoryTable* factories_; \
static baseT* lookup(const std::string& key); \
static std::vector<std::string> factoryToC()



#define defineFactoryTable(baseT, argList, parList) \
 baseT::Factory::~Factory() {} \
 baseT* baseT::lookup(const std::string& key , argList) \
 { \
   baseT::FactoryTable::const_iterator i = baseT::factories_->find(key); \
   if (i==baseT::factories_->end()) \
    throw insight::Exception("Could not lookup type "+key+" in factory table of type " +#baseT); \
   return (*i->second)( parList ); \
 } \
 std::vector<std::string> baseT::factoryToC() \
 { \
   std::vector<std::string> toc; \
   for (const FactoryTable::value_type& e: *factories_) \
   { toc.push_back(e.first); } \
   return toc; \
 } \
 baseT::FactoryTable* baseT::factories_=nullptr

 
 
 
#define defineFactoryTableNoArgs(baseT) \
 baseT::Factory::~Factory() {} \
 baseT* baseT::lookup(const std::string& key) \
 { \
   baseT::FactoryTable::const_iterator i = baseT::factories_->find(key); \
   if (i==baseT::factories_->end()) \
    throw insight::Exception("Could not lookup type "+key+" in factory table of type " +#baseT); \
  return (*i->second)(); \
 } \
 std::vector<std::string> baseT::factoryToC() \
 { \
   std::vector<std::string> toc; \
   for (const FactoryTable::value_type& e: *factories_) \
   { toc.push_back(e.first); } \
   return toc; \
 } \
 baseT::FactoryTable* baseT::factories_=nullptr
 
 
 
#define addToFactoryTable(baseT, specT) \
static struct add##specT##To##baseT##FactoryTable \
{\
  add##specT##To##baseT##FactoryTable()\
  {\
    if (!baseT::factories_) \
    {\
     baseT::factories_=new baseT::FactoryTable(); \
    } \
      \
    std::string key(specT::typeName); \
    (*baseT::factories_)[key]=new baseT::SpecFactory<specT>();\
  }\
  ~add##specT##To##baseT##FactoryTable()\
  {\
    std::string key(specT::typeName); \
    baseT::FactoryTable::iterator k=baseT::factories_->find(key); \
    delete k->second; \
    baseT::factories_->erase(k); \
    if (baseT::factories_->size()==0) \
    { \
     delete baseT::factories_;\
    }\
  }\
} v_add##specT##To##baseT##FactoryTable




#define declareStaticFunctionTable(Name, ReturnT) \
 typedef boost::function0<ReturnT> Name##Ptr; \
 typedef std::map<std::string,Name##Ptr> Name##FunctionTable; \
 static Name##FunctionTable* Name##Functions_; \
 static ReturnT Name(const std::string& key)
 
#define declareStaticFunctionTableWithArgs(Name, ReturnT, argTypeList, argList) \
 typedef boost::function<ReturnT(argTypeList)> Name##Ptr; \
 typedef std::map<std::string,Name##Ptr> Name##FunctionTable; \
 static Name##FunctionTable* Name##Functions_; \
 static ReturnT Name(const std::string& key, argList)

 

#define defineStaticFunctionTable(baseT, Name, ReturnT) \
 ReturnT baseT::Name(const std::string& key) \
 { \
   if (baseT::Name##Functions_) { \
   baseT::Name##FunctionTable::const_iterator i = baseT::Name##Functions_->find(key); \
  if (i==baseT::Name##Functions_->end()) \
    throw insight::Exception("Could not lookup static function #Name for class "+key+" in table of type " #baseT); \
  return i->second(); \
  } else  {\
    throw insight::Exception("Static function table of type " #baseT "is empty!"); \
  }\
 } \
 baseT::Name##FunctionTable* baseT::Name##Functions_ =nullptr
 
 
#define defineStaticFunctionTableWithArgs(baseT, Name, ReturnT, argList, parList) \
 ReturnT baseT::Name(const std::string& key, argList) \
 { \
   if (baseT::Name##Functions_) { \
   baseT::Name##FunctionTable::const_iterator i = baseT::Name##Functions_->find(key); \
  if (i==baseT::Name##Functions_->end()) \
    throw insight::Exception("Could not lookup static function #Name for class "+key+" in table of type " #baseT); \
  return i->second(parList); \
  } else  {\
    throw insight::Exception("Static function table of type " #baseT "is empty!"); \
  }\
 } \
 baseT::Name##FunctionTable* baseT::Name##Functions_ =nullptr


 
#define addToStaticFunctionTable(baseT, specT, Name) \
static struct add##specT##To##baseT##Name##FunctionTable \
{\
  add##specT##To##baseT##Name##FunctionTable()\
  {\
    if (!baseT::Name##Functions_) \
    {\
     baseT::Name##Functions_=new baseT::Name##FunctionTable(); \
    } \
    std::string key(specT::typeName_()); \
    (*baseT::Name##Functions_)[key]=&(specT::Name);\
  }\
} v_add##specT##To##baseT##Name##FunctionTable


#define addStandaloneFunctionToStaticFunctionTable(baseT, specT, Name, FuncName) \
static struct add##specT##To##baseT##Name##FunctionTable \
{\
  add##specT##To##baseT##Name##FunctionTable()\
  {\
    if (!baseT::Name##Functions_) \
    {\
     baseT::Name##Functions_=new baseT::Name##FunctionTable(); \
    } \
    std::string key(specT::typeName_()); \
    (*baseT::Name##Functions_)[key]=&(FuncName);\
  }\
} v_add##specT##To##baseT##Name##FunctionTable



/**
 * declare a class of object which can be parameterized by a single ParameterSet,
 * dynamically selected by a SelectableSubsetParameter
 * 
 * derived classes need to define:
 *  * constructor with single ParameterSet argument for creation
 *  * static ParameterSet defaultParameters() // return a default ParameterSet
 *  * ParameterSet getParameters() // returning the active ParameterSet of an instantiated object
 */
#define declareDynamicClass(baseT) \
    declareFactoryTable ( baseT, LIST ( const ParameterSet& p ), LIST ( p ) ); \
    declareStaticFunctionTable ( defaultParameters, ParameterSet ); \
    static std::shared_ptr<baseT> create ( const SelectableSubsetParameter& ssp ); \
    virtual ParameterSet getParameters() const =0
    
#define defineDynamicClass(baseT) \
    defineFactoryTable(baseT, LIST(const ParameterSet& ps), LIST(ps));\
    std::shared_ptr<baseT> baseT::create(const SelectableSubsetParameter& ssp) \
    { return std::shared_ptr<baseT>( lookup(ssp.selection(), ssp()) ); } \
    defineStaticFunctionTable(baseT, defaultParameters, ParameterSet)

}

#endif // INSIGHT_FACTORY_H
