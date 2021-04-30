#ifndef INSIGHT_CACHEABLEENTITY_H
#define INSIGHT_CACHEABLEENTITY_H

#include <iostream>
#include <memory>
#include <map>
#include <type_traits>



#include "base/boost_include.h"
#include "boost/enable_shared_from_this.hpp"
#include <boost/preprocessor.hpp>
#include "base/cacheableentityhashes.h"



namespace insight {



class CacheableEntity
    : public std::enable_shared_from_this<CacheableEntity>
{

protected:
  static std::map<size_t, std::weak_ptr<CacheableEntity> > cache;

  size_t hash_=0;
  virtual size_t calcHash() const =0;

  bool validResult_=false;
  void assertValidResult() const;

  virtual void build() =0;

public:
  virtual ~CacheableEntity();

  inline bool isValid() const { return validResult_; }
  size_t getHash() const;
  inline static size_t cacheSize() { return cache.size(); }

};

} // namespace insight


#define INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple) \
  BOOST_PP_TUPLE_ELEM( 3, 0, elemtuple )

#define INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple) \
  BOOST_PP_TUPLE_ELEM( 3, 1, elemtuple )

#define INSIGHT_CACHEABLEENTITY_ELEM_VAL(elemtuple) \
  BOOST_PP_TUPLE_ELEM( 3, 2, elemtuple )

#define INSIGHT_CACHEABLEENTITY_ELEM_DEF(r, data, elemtuple) \
  INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)  INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple);

#define INSIGHT_CACHEABLEENTITY_ELEM_DEFS(elemlist) \
  BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_DEF, _, elemlist)

// ==== input value get function
#define INSIGHT_CACHEABLEENTITY_ELEM_GETINPUT_FUNCTION(r, data, elemtuple) \
  const INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)& \
   BOOST_PP_CAT(get_,INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple))() const \
  { \
   return INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple); \
  }
// ====

#define INSIGHT_CACHEABLEENTITY_ELEM_GETINPUT_FUNCTIONS(elemlist) \
  BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_GETINPUT_FUNCTION, _, elemlist)


// ==== constructor parameter
#define INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETER(r, data, elemtuple) \
  (const INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)& BOOST_PP_CAT(_,INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple)) = INSIGHT_CACHEABLEENTITY_ELEM_VAL(elemtuple))
// ====

#define INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETERS(elemlist) \
  BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETER, _, elemlist))

#define INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETER_ASSGN(r, data, elemtuple) \
  INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple) = BOOST_PP_CAT(_,INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple));

#define INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETERS_ASSGN(elemlist) \
  BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETER_ASSGN, _, elemlist)

#define INSIGHT_CACHEABLEENTITY_ELEM_PARAMETER_ENTRY(r, data, elemtuple) \
  (BOOST_PP_CAT(_,INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple)))

#define INSIGHT_CACHEABLEENTITY_ELEM_PARAMETER_ENTRIES(elemlist) \
  BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_PARAMETER_ENTRY, _, elemlist))


#define INSIGHT_CACHEABLEENTITY_xstr(a) INSIGHT_CACHEABLEENTITY_str(a)
#define INSIGHT_CACHEABLEENTITY_str(a) #a
//==== calc hash
#define INSIGHT_CACHEABLEENTITY_ELEM_CALC_HASH(r, data, elemtuple) \
  boost::hash_combine(h, \
   boost::hash<INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)>() \
     (INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple))); \
  std::cout<< INSIGHT_CACHEABLEENTITY_xstr(INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)) "  hash="<<boost::hash<INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)>() \
  (INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple))<<std::endl;
//====

#define INSIGHT_CACHEABLEENTITY_ELEM_CALC_HASHES(elemlist) \
  BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_CALC_HASH, _, elemlist)

#define INSIGHT_CACHEABLEENTITY_INPUTS(className, keepLastNInstances, elements) \
private: \
  INSIGHT_CACHEABLEENTITY_ELEM_DEFS(elements) \
  className( INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETERS(elements) ) \
  { INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETERS_ASSGN(elements) } \
  static std::queue<std::shared_ptr<className> >& lastInstances() { static std::queue<std::shared_ptr<className> > lc; return lc; } \
public: \
  static std::shared_ptr<className> \
  create( INSIGHT_CACHEABLEENTITY_ELEM_CONSTRUCTOR_PARAMETERS(elements) ) \
  { \
    std::shared_ptr<className> pp( \
      new className(INSIGHT_CACHEABLEENTITY_ELEM_PARAMETER_ENTRIES(elements)) \
    ); \
    auto cacheIndex = cache.find(pp->getHash()); \
    if ( cacheIndex!=cache.end() ) \
    { \
     std::cout<<"CacheableEntity: restore " #className " " << pp->getHash()<<" from cache"<<std::endl; \
     pp=std::dynamic_pointer_cast<className>(cacheIndex->second.lock()); \
    } else {\
     cache.emplace(pp->getHash(), pp); \
     auto h=pp->getHash(); \
     std::cout<<"added " #className " "<<h<<" to cache, cache size now " << cache.size() <<std::endl; \
     if (int(lastInstances().size())>keepLastNInstances-1) { \
      std::cout<<" removing "<<lastInstances().front()->getHash() << " from lastInstances "<<std::flush; \
      while (int(lastInstances().size())>keepLastNInstances-1) { std::cout<<"."<<std::flush; lastInstances().pop(); }\
      std::cout<<"cache size now " << cache.size() <<std::endl; \
     } \
     if (keepLastNInstances>0) { \
      std::cout<<"adding " #className " "<<h<<" to lastInstances."<<std::endl; \
      lastInstances().push(pp); \
     } \
    }\
    return pp; \
  } \
  \
  INSIGHT_CACHEABLEENTITY_ELEM_GETINPUT_FUNCTIONS(elements) \
  size_t calcHash() const override { \
   size_t h=0; \
   std::cout<<"calc hash of " #className ":" << std::endl;\
   boost::hash_combine(h, boost::hash<std::string>()(#className));\
   std::cout<<"className " #className "  hash="<<boost::hash<std::string>()(#className)<<std::endl;\
   INSIGHT_CACHEABLEENTITY_ELEM_CALC_HASHES(elements) \
   return h;\
  }

// ==== output get function
#define INSIGHT_CACHEABLEENTITY_ELEM_GETOUTPUT_FUNCTION(r, data, elemtuple) \
  const INSIGHT_CACHEABLEENTITY_ELEM_TYPE(elemtuple)& BOOST_PP_CAT(get_,INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple))() const \
  { \
    assertValidResult(); \
    return INSIGHT_CACHEABLEENTITY_ELEM_NAME(elemtuple); \
  }
// ====

#define INSIGHT_CACHEABLEENTITY_ELEM_GETOUTPUT_FUNCTIONS(elemlist) \
  BOOST_PP_SEQ_FOR_EACH(INSIGHT_CACHEABLEENTITY_ELEM_GETOUTPUT_FUNCTION, _, elemlist)

#define INSIGHT_CACHEABLEENTITY_OUTPUTS(elements) \
  INSIGHT_CACHEABLEENTITY_ELEM_DEFS(elements) \
  INSIGHT_CACHEABLEENTITY_ELEM_GETOUTPUT_FUNCTIONS(elements)



#endif // INSIGHT_CACHEABLEENTITY_H
