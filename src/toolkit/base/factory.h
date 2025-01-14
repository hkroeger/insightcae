#ifndef INSIGHT_FACTORY_H
#define INSIGHT_FACTORY_H

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


/*
 * MUST BE HEADER ONLY! (used in PDL without linking toolkit lib!)
 */

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <memory>

namespace std {

#if __cplusplus <= 201103L

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif


}

namespace insight {



#define declareType(typenameStr) \
 static const char* typeName_() { return typenameStr; } \
 virtual const std::string& type() const { return typeName; } \
 static const std::string typeName
 
 
  
#define defineType(T) \
 const std::string T::typeName( T::typeName_() )

#define defineTemplateType(T) \
 template<> const std::string T::typeName( T::typeName_() )

 
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
 static bool has_factory(const std::string& key); \
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
static bool has_factory(const std::string& key); \
static baseT* lookup(const std::string& key); \
static std::vector<std::string> factoryToC()



#define defineFactoryTable(baseT, argList, parList) \
 baseT::Factory::~Factory() {} \
 bool baseT::has_factory(const std::string& key) \
 {\
  if (factories_) { \
   auto i = baseT::factories_->find(key); \
   return (i!=baseT::factories_->end()); \
  }\
  return false;\
 }\
 baseT* baseT::lookup(const std::string& key , argList) \
 { \
  if (factories_) { \
   baseT::FactoryTable::const_iterator i = baseT::factories_->find(key); \
   if (i==baseT::factories_->end()) \
    throw std::runtime_error("Could not lookup type \""+key+"\" in factory table of type \"" #baseT "\"" ); \
   return (*i->second)( parList ); \
  } \
  else throw std::runtime_error("Factory table of type \"" #baseT "\" is empty!" ); \
 } \
 std::vector<std::string> baseT::factoryToC() \
 { \
   std::vector<std::string> toc; \
   if (factories_) { \
    for (const FactoryTable::value_type& e: *factories_) \
    { toc.push_back(e.first); } \
   } \
   return toc; \
 } \
 baseT::FactoryTable* baseT::factories_=nullptr




template<const std::string* typeName, class FunctionReturnType = void, class ...FunctionArgs>
class StaticFunctionTable
    : public std::map<
          std::string,
          std::function<
              FunctionReturnType( FunctionArgs... )
              >
          >
{


public:
    StaticFunctionTable()
    {}

    typedef std::function<
        FunctionReturnType( FunctionArgs... )
        > Function;


    // //not working as expected
    // bool has(const std::string& key) const
    // {
    //     return this->count(key)>0;
    // }

    std::vector<std::string> ToC() const
    {
        std::vector<std::string> toc;
        std::transform(
            this->begin(), this->end(),
            std::back_inserter(toc),
            [](const typename std::map<std::string, Function>::value_type& e)
            { return e.first; }
            );
        return toc;
    }

    Function lookup(
        const std::string& key) const
    {
        auto i=this->find(key);

        if (i==this->end())
        {
            throw std::runtime_error(
                "Could not lookup type \""
                + key +
                "\" in factory table of "
                + *typeName );
        }

        return i->second;
    }

    typename Function::result_type operator()(
        const std::string& key,
        FunctionArgs&&... addArgs) const
    {
        return std::move(
            lookup(key)( std::forward<FunctionArgs>(addArgs)... )
            );
    }


    template<class Instance>
    struct Add {
        // std::unique_ptr<> createInstance
        Add(StaticFunctionTable& table, Function f) {
            table.emplace(
                Instance::typeName_(), f
                );
        }
    };

};



#define declareStaticFunctionTable2(TYPE, NAME)                         \
static TYPE& NAME##_table();                                            \
template<class ...FunctionArgs>                                         \
static TYPE::Function::result_type                                      \
NAME(const TYPE::key_type& key, FunctionArgs&&... addArgs)              \
{ return NAME##_table()( key, std::forward<FunctionArgs>(addArgs)...); }


#define defineStaticFunctionTable2(BASE, TYPE, NAME)                    \
BASE::TYPE& BASE::NAME##_table() {                                      \
 static TYPE theInstance;                                               \
 return theInstance;                                                    \
}


#define addToStaticFunctionTable2(BASE, TYPE, NAME, DERIVED, OBJECT)    \
BASE::TYPE::Add<DERIVED>                                                \
    add##DERIVED##To##NAME##In##BASE(                                             \
        BASE::NAME##_table(),                                           \
        OBJECT )

#define addToFactoryTable2(BASE, TYPE, NAME, DERIVED)                   \
BASE::TYPE::Add<DERIVED>                                                \
    add##DERIVED##To##NAME##In##BASE(                                   \
      BASE::NAME##_table() )



template<const std::string* typeName, class BaseClass, class ...FunctionArgs>
class Factory
    : public StaticFunctionTable<typeName, std::unique_ptr<BaseClass>, FunctionArgs...>
{

public:

    typedef StaticFunctionTable<typeName, std::unique_ptr<BaseClass>, FunctionArgs...> FactoryBase;

    template<class Instance>
    struct Add
        : public FactoryBase::template Add<Instance>
    {
        Add(Factory& factory)
        : FactoryBase::template Add<Instance>(
            factory,
            &std::make_unique<Instance, FunctionArgs...> )
        {}
    };
};





 
#define defineFactoryTableNoArgs(baseT) \
 baseT::Factory::~Factory() {} \
 bool baseT::has_factory(const std::string& key) \
 {\
   if (factories_) { \
    auto i = baseT::factories_->find(key); \
    return (i!=baseT::factories_->end()); \
   }\
   return false;\
 }\
 baseT* baseT::lookup(const std::string& key) \
 { \
   if (factories_) { \
    baseT::FactoryTable::const_iterator i = baseT::factories_->find(key); \
    if (i==baseT::factories_->end()) \
     throw std::runtime_error("Could not lookup type \""+key+"\" in factory table of type \"" #baseT "\"" ); \
    return (*i->second)(); \
   } \
   else throw std::runtime_error("Factory table of type \"" #baseT "\" is empty!" ); \
 } \
 std::vector<std::string> baseT::factoryToC() \
 { \
   std::vector<std::string> toc; \
   if (factories_) { \
    for (const FactoryTable::value_type& e: *factories_) \
    { toc.push_back(e.first); } \
   } \
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
    if (k!=baseT::factories_->end()) { \
     delete k->second; \
     baseT::factories_->erase(k); \
     if (baseT::factories_->size()==0) \
     { \
      delete baseT::factories_;\
      baseT::factories_=nullptr;\
     }\
    } else {\
      std::cerr<<"Internal error: attempt to remove factory of "<<key<<" twice."<<std::endl;\
    }\
  }\
} v_add##specT##To##baseT##FactoryTable




#define declareStaticFunctionTable(Name, ReturnT) \
 typedef ReturnT Name##ReturnType; \
 typedef std::function<ReturnT(void)> Name##Ptr; \
 typedef std::map<std::string,Name##Ptr> Name##FunctionTable; \
 static Name##FunctionTable* Name##Functions_; \
 static bool has_##Name(const std::string& key); \
static ReturnT Name##For(const std::string& key)
 
#define declareStaticFunctionTableWithArgs(Name, ReturnT, argTypeList, argList) \
 typedef ReturnT Name##ReturnType; \
 typedef std::function<ReturnT(argTypeList)> Name##Ptr; \
 typedef std::map<std::string,Name##Ptr> Name##FunctionTable; \
 static Name##FunctionTable* Name##Functions_; \
 static bool has_##Name(const std::string& key); \
static ReturnT Name##For(const std::string& key, argList)

 

#define defineStaticFunctionTable(baseT, Name, ReturnT) \
 typedef ReturnT Name##ReturnType; \
 bool baseT::has_##Name(const std::string& key) \
 { \
  if (baseT::Name##Functions_) { \
      auto i = baseT::Name##Functions_->find(key); \
      if (i!=baseT::Name##Functions_->end()) \
       return true; \
  } \
  return false; \
 } \
ReturnT baseT::Name##For(const std::string& key) \
 { \
   if (baseT::Name##Functions_) { \
   baseT::Name##FunctionTable::const_iterator i = baseT::Name##Functions_->find(key); \
  if (i==baseT::Name##Functions_->end()) \
    throw std::runtime_error("Could not lookup static function \"" #Name "\" for class \""+key+"\" in table of type \"" #baseT "\""); \
  return i->second(); \
  } else  {\
    throw std::runtime_error("Static function table of type \"" #baseT "\" is empty!"); \
  }\
 } \
 baseT::Name##FunctionTable* baseT::Name##Functions_ =nullptr
 
 
#define defineStaticFunctionTableWithArgs(baseT, Name, ReturnT, argList, parList) \
 bool baseT::has_##Name(const std::string& key) \
 { \
        if (baseT::Name##Functions_) { \
            auto i = baseT::Name##Functions_->find(key); \
            if (i!=baseT::Name##Functions_->end()) \
            return true; \
    } \
        return false; \
 } \
ReturnT baseT::Name##For(const std::string& key, argList) \
 { \
   if (baseT::Name##Functions_) { \
   baseT::Name##FunctionTable::const_iterator i = baseT::Name##Functions_->find(key); \
  if (i==baseT::Name##Functions_->end()) \
    throw std::runtime_error("Could not lookup static function \"" #Name "\" for class \""+key+"\" in table of type \"" #baseT "\""); \
  return i->second(parList); \
  } else  {\
    throw std::runtime_error("Static function table of type \"" #baseT "\" is empty!"); \
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
    (*baseT::Name##Functions_)[key]=&specT::Name;\
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
    (*baseT::Name##Functions_)[key]=&FuncName;\
  }\
} v_add##specT##To##baseT##Name##FunctionTable

#define addFunctionToStaticFunctionTable(baseT, specT, Name, Function) \
static struct add##specT##To##baseT##Name##FunctionTable \
    {\
        add##specT##To##baseT##Name##FunctionTable()\
        {\
            if (!baseT::Name##Functions_) \
            {\
                baseT::Name##Functions_=new baseT::Name##FunctionTable(); \
} \
    std::string key(specT::typeName_()); \
    (*baseT::Name##Functions_)[key]=Function;\
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
    declareFactoryTable ( baseT, LIST ( ParameterSetInput&& ip ), LIST ( std::move(ip) ) ); \
    declareStaticFunctionTable ( defaultParameters, std::unique_ptr<ParameterSet> ); \
    static std::shared_ptr<baseT> create ( const SelectableSubsetParameter& ssp );

#define declareDynamicClass_implGetParameters(baseT) \
declareFactoryTable ( baseT, LIST ( ParameterSetInput&& ip ), LIST ( std::move(ip) ) ); \
    declareStaticFunctionTable ( defaultParameters, std::unique_ptr<ParameterSet> ); \
    static std::shared_ptr<baseT> create ( const SelectableSubsetParameter& ssp );

#define defineDynamicClass(baseT) \
    defineFactoryTable(baseT, LIST(ParameterSetInput&& ip), LIST(std::move(ip)));\
    std::shared_ptr<baseT> baseT::create(const SelectableSubsetParameter& ssp) \
    { return std::shared_ptr<baseT>( lookup(ssp.selection(), ssp()) ); } \
    defineStaticFunctionTable(baseT, defaultParameters, std::unique_ptr<ParameterSet>)


#define CREATE_FUNCTION(DerivedClass) \
template <typename... T> \
    static std::shared_ptr<DerivedClass> create(T /*&&*/...args) \
{ \
        DerivedClass *ptr = new DerivedClass(::std::forward<T>(args)...); \
        return std::shared_ptr<DerivedClass>(ptr); \
}

#define REUSE_CTOR(BaseClass, DerivedClass) \
template<class ...Args> \
DerivedClass(Args&&... addArgs) \
    : BaseClass( std::forward<Args>(addArgs)... ) \
{}

}

#endif // INSIGHT_FACTORY_H
