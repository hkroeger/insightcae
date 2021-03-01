#ifndef INSIGHT_CAD_FEATURECACHE_H
#define INSIGHT_CAD_FEATURECACHE_H

//#include "cadfeature.h"

#include <map>
#include <set>
#include <memory>

#include "base/exception.h"

namespace insight {
namespace cad {

class Feature;

class FeatureCache
: public std::map<size_t, std::weak_ptr<Feature> >
{

public:
  FeatureCache();
  ~FeatureCache();

  void cleanup();

  static std::string featureInfo(FeaturePtr);
  void printSummary(std::ostream& os, bool detailed=false) const;

  void insert(std::shared_ptr<Feature> p);
  bool contains(size_t hash) const;

  template<class T>
  std::shared_ptr<T> markAsUsed(size_t hash)
  {
    iterator i=this->find(hash);

    if (i==end())
      throw insight::Exception
      (
        "requested entry in CAD feature cache is not found!"
      );

    auto wp=i->second;

    insight::assertion( !i->second.expired(),
                        "cache contained expired element!" );

    auto sp=wp.lock();

    std::shared_ptr<T> cp
    (
      std::dynamic_pointer_cast<T>( sp )
    );

    if (!cp)
      throw insight::Exception
      (
        "requested entry in CAD feature cache found,"
        " but is of wrong type! Found in cache: "+featureInfo(sp)
      );

    return cp;
  }

};

extern FeatureCache cache;

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_FEATURECACHE_H
