#include "patch.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

namespace insight {
namespace bmd {


Patch::Patch(std::string typ)
: typ_(typ)
{}

Patch::~Patch()
{}

void Patch::addFace(Point c1, Point c2, Point c3, Point c4)
{
  PointList f;
  f+=c1,c2,c3,c4;
  faces_.push_back(f);
}

void Patch::addFace(const PointList& corners)
{
  faces_.push_back(corners);
}

void Patch::appendPatch(const Patch& opatch)
{
  faces_.insert(faces_.end(), opatch.faces_.begin(), opatch.faces_.end());
}

void Patch::clear()
{
  faces_.clear();
}


Patch* Patch::transformed(const arma::mat& tm, bool inv, const arma::mat trans) const
{
  std::unique_ptr<Patch> np(new Patch(typ_));
  for ( const PointList& pl: faces_)
  {
    PointList npl;
    if (inv)
    {
      for (PointList::const_reverse_iterator it=pl.rbegin(); it!=pl.rend(); it++)
      {
        npl.push_back( tm * (*it) +trans );
      }
    }
    else
    {
      for (PointList::const_iterator it=pl.begin(); it!=pl.end(); it++)
      {
        npl.push_back( tm * (*it) +trans);
      }
    }
    np->addFace(npl);
  }
  return np.release();
}

Patch* Patch::clone() const
{
  std::unique_ptr<Patch> p(new Patch(typ_));
  for (const auto&f: faces_)
    p->addFace(f);
  return p.release();
}





std::vector<OFDictData::data>
Patch::bmdEntry(const PointMap& allPoints, const std::string& name, int OFversion) const
{
  std::vector<OFDictData::data> retval;

  if (OFversion<210)
  {
    std::ostringstream oss;
    oss << typ_ << " " << name << "\n(\n";
    for (FaceList::const_iterator i=faces_.begin(); i!=faces_.end(); i++)
    {
      oss << " ("
      << allPoints.find((*i)[0])->second
      << " "
      << allPoints.find((*i)[1])->second
      << " "
      << allPoints.find((*i)[2])->second
      << " "
      << allPoints.find((*i)[3])->second
          << ")\n";
    }
    oss << ")\n";
    retval.push_back( OFDictData::data(oss.str()) );
  }
  else
  {
    if (typ_=="cyclic")
    {
      int h=faces_.size()/2;
      {
    OFDictData::dict d;
    d["type"]="cyclic";
    OFDictData::list fl;
    for (size_t i=0; i<h; i++)
    {
      OFDictData::list vl;
      vl += allPoints.find(faces_[i][0])->second,
        allPoints.find(faces_[i][1])->second,
        allPoints.find(faces_[i][2])->second,
        allPoints.find(faces_[i][3])->second;
      fl.push_back(vl);
    }
    d["faces"]=fl;
    d["neighbourPatch"]=name+"_half1";
    retval.push_back( OFDictData::data(name+"_half0") );
    retval.push_back( d );
      }
      {
    OFDictData::dict d;
    d["type"]="cyclic";
    OFDictData::list fl;
    for (size_t i=h; i<faces_.size(); i++)
    {
      OFDictData::list vl;
      vl += (allPoints.find(faces_[i][0])->second),
        (allPoints.find(faces_[i][1])->second),
        (allPoints.find(faces_[i][2])->second),
        (allPoints.find(faces_[i][3])->second);
      fl.push_back(vl);
    }
    d["faces"]=fl;
    d["neighbourPatch"]=name+"_half0";
    retval.push_back( OFDictData::data(name+"_half1") );
    retval.push_back( d );
      }
    }
    else
    {
      OFDictData::dict d;
      d["type"]=typ_;
      OFDictData::list fl;
      for (FaceList::const_iterator i=faces_.begin(); i!=faces_.end(); i++)
      {
    OFDictData::list vl;
    vl += allPoints.find((*i)[0])->second,
          allPoints.find((*i)[1])->second,
          allPoints.find((*i)[2])->second,
          allPoints.find((*i)[3])->second;
    fl.push_back(vl);
      }
      d["faces"]=fl;
      retval.push_back( OFDictData::data(name) );
      retval.push_back( d );
    }
  }

  return retval;
}


} // namespace bmd
} // namespace insight
