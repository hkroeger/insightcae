#include "cadfeature.h"
#include "featurecache.h"
#include "cadexception.h"
#include "datum.h"
#include "base/tools.h"
#include "base/warningdispatcher.h"

namespace insight {
namespace cad {


FeatureCache::FeatureCache()
{}

FeatureCache::~FeatureCache()
{}


std::string FeatureCache::featureInfo(std::shared_ptr<const Feature> fp)
{
  return fp->featureSymbolName() +
      " (type " + fp->type() + ", hash=" + toString(fp->hash()) + ")";
}


std::shared_ptr<Feature> FeatureCache::lookup(size_t hash, const std::type_info& type)
{
  std::lock_guard<std::mutex> l(mtx_);

  auto i=entries_.find(hash);
  if (i==entries_.end())
    return nullptr;

  auto sp=i->second.lock();
  if (!sp)
  {
    entries_.erase(i);
    return nullptr;
  }

  if (typeid(*sp)!=type)
  {
    insight::Warning(
          "hash collision in CAD feature cache: requested type %s, found %s. "
          "Ignoring cache entry.",
          type.name(), featureInfo(sp).c_str() );
    return nullptr;
  }

  return sp;
}


void FeatureCache::insert(std::shared_ptr<Feature> p)
{
  size_t h=p->hash();
  std::lock_guard<std::mutex> l(mtx_);
  entries_[h]=p;
}


void FeatureCache::release(size_t hash, const Feature* f)
{
  if (hash==0) return;

  std::lock_guard<std::mutex> l(mtx_);

  auto i=entries_.find(hash);
  if (i==entries_.end())
    return;

  auto sp=i->second.lock();
  if (!sp || sp.get()==f)
  {
    entries_.erase(i);
  }
}


void FeatureCache::clear()
{
  std::lock_guard<std::mutex> l(mtx_);
  entries_.clear();
}


size_t FeatureCache::size() const
{
  std::lock_guard<std::mutex> l(mtx_);
  return entries_.size();
}


void FeatureCache::printSummary(std::ostream& os, bool detailed) const
{
  std::lock_guard<std::mutex> l(mtx_);
  os<<"Cache contains "<<entries_.size()<<" entities."<<std::endl;
  if (detailed)
  {
    for (const auto&i: entries_)
    {
      os << "  " << i.first <<" \t";
      if (auto fp=i.second.lock())
      {
        os << featureInfo(fp) << "\n";
      }
      else
      {
        os << "EXPIRED!\n";
      }
    }
  }
}



FeatureCache cache;


} // namespace cad
} // namespace insight
