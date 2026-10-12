#ifndef INSIGHT_CAD_FEATURECACHE_H
#define INSIGHT_CAD_FEATURECACHE_H

#include <map>
#include <memory>
#include <mutex>
#include <typeinfo>
#include <iostream>

#include "base/exception.h"

namespace insight {
namespace cad {

class Feature;


/**
 * Registry of built features, indexed by their input hash.
 *
 * Entries are held weakly: the cache never keeps a feature alive.
 * When a new feature with an identical hash is built, its results
 * are copied from the registered feature (see Feature::restoreFromCache)
 * and the new feature takes over the entry.
 *
 * All member functions are thread-safe.
 */
class FeatureCache
{
  typedef std::map<size_t, std::weak_ptr<Feature> > Entries;

  mutable std::mutex mtx_;
  Entries entries_;

public:
  FeatureCache();
  ~FeatureCache();

  static std::string featureInfo(std::shared_ptr<const Feature>);

  /**
   * returns the live feature registered for hash, if it is of the given type.
   * Returns null otherwise. Expired entries are removed.
   */
  std::shared_ptr<Feature> lookup(size_t hash, const std::type_info& type);

  /**
   * registers p under its hash. An existing entry is replaced.
   */
  void insert(std::shared_ptr<Feature> p);

  /**
   * removes the entry for hash, but only if it refers to f or is expired.
   * Entries of other live features with the same hash are kept.
   */
  void release(size_t hash, const Feature* f);

  void clear();
  size_t size() const;

  void printSummary(std::ostream& os, bool detailed=false) const;
};


extern FeatureCache cache;


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_FEATURECACHE_H
