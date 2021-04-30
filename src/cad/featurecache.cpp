#include "cadfeature.h"
#include "featurecache.h"

namespace insight {
namespace cad {


FeatureCache::FeatureCache()
{}

FeatureCache::~FeatureCache()
{}

void FeatureCache::cleanup()
{
  std::set<size_t> toBeDeleted;
  for (const auto& i: *this)
  {
    if (i.second.expired())
      toBeDeleted.insert(i.first);
  }
  for (const auto& i: toBeDeleted)
  {
    erase(i);
  }

  std::cout<<"== After cache cleanup: cache summary =="<<std::endl;
  printSummary(std::cout);
}

std::string FeatureCache::featureInfo(FeaturePtr fp)
{
  return fp->featureSymbolName() +
      " (type " + fp->type() + ", hash=" + boost::lexical_cast<std::string>(fp->hash()) + ")";
}


void FeatureCache::printSummary(std::ostream& os, bool detailed) const
{
  os<<"Cache contains "<<size()<<" entities."<<std::endl;
  if (detailed)
  {
    for (const auto&i: *this)
    {
      auto wp=i.second;
      os << "  " << i.first <<" \t";
      if(wp.expired())
      {
        os<<"EXPIRED!\n";
      }
      else
      {
        auto fp=wp.lock();
        os <<featureInfo(fp) << "\n";
      }
    }
  }
}

void FeatureCache::insert(FeaturePtr p)
{
  size_t h=p->hash();
  const_iterator i=find(h);
  if (i!=end())
    {
      insight::assertion( !i->second.expired(),
                          "cache contained expired element!" );

      auto sp=i->second.lock();

      std::ostringstream msg;
      msg<<"Internal error: trying to insert feature into CAD feature cache twice!\n";
      msg<<"feature to insert: hash="<<h<<" (of type "<<p->type()<<" named \""<<p->featureSymbolName()<<"\")\n";
      msg<<"present feature: "<<featureInfo(sp)<<"\n";
      throw insight::cad::CADException(p, msg.str());
    }
  (*this)[h]=p;
}


bool FeatureCache::contains(size_t hash) const
{
  return ( this->find(hash) != end() );
}



FeatureCache cache;


} // namespace cad
} // namespace insight
