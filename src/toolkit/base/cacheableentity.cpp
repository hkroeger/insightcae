#include "cacheableentity.h"

namespace insight {


std::map<size_t, std::weak_ptr<CacheableEntity> > CacheableEntity::cache;


void CacheableEntity::assertValidResult() const
{
  if (validResult_) return;
  else
  {
    auto ncthis=const_cast<CacheableEntity*>(this);
    ncthis->build();
    ncthis->validResult_=true;
  }
}




CacheableEntity::~CacheableEntity()
{
  if ( (hash_>0) && isValid() )
  {
    auto i=cache.find(hash_);
    if (i!=cache.end())
    {
      cache.erase(i);
      std::cout<<" remove from cache "<<hash_<<std::endl;
    }
  }
}




size_t CacheableEntity::getHash() const
{
  if (hash_==0)
  {
    auto ncthis=const_cast<CacheableEntity*>(this);
    ncthis->hash_=ncthis->calcHash();
  }
  return hash_;
}



} // namespace insight


