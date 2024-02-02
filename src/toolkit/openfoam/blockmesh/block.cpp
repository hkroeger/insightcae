#include "block.h"
#include "openfoam/blockmesh.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=


namespace insight {
namespace bmd {




Block::Block
(
  PointList corners,
  int resx, int resy, int resz,
  GradingList grading,
  std::string zone,
  bool inv
)
: corners_(corners),
grading_(grading),
zone_(zone),
inv_(inv)
{
  resolution_ += resx, resy, resz;
  if (inv) swapGrad();
}




Block::~Block()
{}




Block* Block::transformed(const arma::mat& tm, bool inv, const arma::mat trans) const
{
  PointList p2;
  insight::dbg()<<"#corners="<<corners_.size()<<std::endl;
  for ( const Point& p: corners_ )
  {
    p2 += tm*p+trans;
  }
  return new Block
  (
    p2,
   resolution_[0], resolution_[1], resolution_[2],
   grading_,
   zone_,
   inv ? (!inv_) : inv_
  );
}




Block* Block::clone() const
{
    return new Block
            (
                corners_,
                resolution_[0], resolution_[1], resolution_[2],
            grading_,
            zone_,
            false
            );
}




const PointList &Block::corners() const
{
    return corners_;
}




void Block::registerPoints(blockMesh& bmd) const
{
  for (PointList::const_iterator i=corners_.begin();
       i!=corners_.end(); i++)
       bmd.addPoint(*i);
}




PointList Block::face(const std::string& id) const
{
  if ((id=="0321") || (id=="A") || (id=="a"))
  {
    return P_4(corners_[0], corners_[3], corners_[2], corners_[1]);
  }
  else if ((id=="2376") || (id=="B") || (id=="b"))
  {
    return P_4(corners_[2], corners_[3], corners_[7], corners_[6]);
  }
  else if ((id=="4567") || (id=="C") || (id=="c"))
  {
    return P_4(corners_[4], corners_[5], corners_[6], corners_[7]);
  }
  else if ((id=="0154") || (id=="D") || (id=="d"))
  {
    return P_4(corners_[0], corners_[1], corners_[5], corners_[4]);
  }
  else if ((id=="0473") || (id=="E") || (id=="e"))
  {
    return P_4(corners_[0], corners_[4], corners_[7], corners_[3]);
  }
  else if ((id=="1265") || (id=="F") || (id=="f"))
  {
    return P_4(corners_[1], corners_[2], corners_[6], corners_[5]);
  }
  else
    throw insight::Exception("Unknown face of block: "+id);

  return PointList();
}




void Block::swapGrad()
{
    typedef std::vector<double> List;

    if (List *gl = boost::get<List>(&grading_[2]))
    {
        if (gl->size()>0)
        {
            for ( double& g: *gl )
            {
                g = 1./g;
            }
        }
    }
    else
    {
        double & g = boost::get<double>(grading_[2]);
        g = 1./g;
    }

    if (List *gl = boost::get<List>(&grading_[0]))
    {
        if (gl->size()==4)
        {
            double tmp0=(*gl)[0];
            double tmp1=(*gl)[1];
            (*gl)[0]=(*gl)[3];
            (*gl)[1]=(*gl)[2];
            (*gl)[3]=tmp0;
            (*gl)[2]=tmp1;
        }
    }

    if (List *gl = boost::get<List>(&grading_[1]))
    {
        if (gl->size()==4)
        {
            double tmp0=(*gl)[0];
            double tmp1=(*gl)[1];
            (*gl)[0]=(*gl)[3];
            (*gl)[1]=(*gl)[2];
            (*gl)[3]=tmp0;
            (*gl)[2]=tmp1;
        }
    }

}




std::vector<OFDictData::data>
Block::bmdEntry(const PointMap& allPoints, int OFversion) const
{
    std::vector<OFDictData::data> retval;
    retval.push_back( OFDictData::data("hex") );

    OFDictData::list cl;
    std::vector<int> ci;
    if (!inv_) ci+=0,1,2,3,4,5,6,7; else ci+=4,5,6,7,0,1,2,3;
    for (auto i: ci)
    {
        cl.push_back(
            OFDictData::data(
                allPoints.find(
                    corners_[i] )->second ) );
    }
    retval.push_back( cl );

    retval.push_back( OFDictData::data(zone_) );

    OFDictData::list rl;
    rl += resolution_[0], resolution_[1], resolution_[2];
    retval.push_back( rl );

    OFDictData::list gl;
    for (const auto& g: grading_)
    {
        if (const auto* grad = boost::get<double>(&g) )
        {
            gl += *grad, *grad, *grad, *grad;
        }
        else if (const auto* gradl = boost::get<std::vector<double> >(&g))
        {
            if (gradl->size()==2)
            {
                gl += (*gradl)[0], (*gradl)[1], (*gradl)[1], (*gradl)[0];
            }
            else if (gradl->size()==4)
            {
                gl += (*gradl)[0], (*gradl)[1], (*gradl)[2], (*gradl)[3];
            }
            else
            {
                throw Exception("Invalid number of grading coefficients given!");
            }
        }
        else
        {
            throw Exception("internal error: unhandled selection!");
        }
    }
    retval.push_back( gl );

    return retval;
}



} // namespace bmd
} // namespace insight
