/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "blockmesh.h"

using namespace std;
using namespace boost::assign;
using namespace arma;

#define SMALL 1e-10


namespace insight
{
  
namespace bmd
{
/*
Point::Point()
{
  (*this) << 0.0 << endr << 0.0 << endr << 0.0 <<endr;
}

Point::Point(const mat& m)
: mat(m)
{
}

Point::Point(double x, double y, double z)
{
  (*this) << x << endr << y << endr << z <<endr;
}*/
/*
    def __str__(self):
        return "(%f %f %f)\n" % (self.x, self.y, self.z)

    def bmdStr(self, pointOffsets):
        return "(%f %f %f)\n" % (
            self.x+pointOffsets[self][0], 
            self.y+pointOffsets[self][1], 
            self.z+pointOffsets[self][2])

    def __hash__(self):
        s=str(self)
        return hash(s)

    def __cmp__(self, other):
        return cmp(hash(self), hash(other))
*//*
void Point::operator=(const arma::mat& m)
{
  mat::operator=(m);
}*/

bool compare(const Point& v1, const Point& v2)
{
  if ( fabs(v1(0) - v2(0))<SMALL )
    {
      if ( fabs(v1(1) - v2(1))<SMALL )
        {
          if (fabs(v1(2)-v2(2))<SMALL)
          {
              //return v1.instance_ < v2.instance_;
              return false;
          }
          else return v1(2)<v2(2);
        }
      else return v1(1)<v2(1);
    }
  else return v1(0)<v2(0);
}


PointList P_4(const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
  return list_of<Point>(p1)(p2)(p3)(p4);
}

PointList P_4(const PointList& pts, int p1, int p2, int p3, int p4)
{
  return P_4(pts[p1], pts[p2], pts[p3], pts[p4]);
}


PointList P_8(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
	      const Point& p5, const Point& p6, const Point& p7, const Point& p8)
{
  return list_of<Point>(p1)(p2)(p3)(p4)(p5)(p6)(p7)(p8);
}


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

Block* Block::transformed(const arma::mat& tm, bool inv) const
{
  PointList p2;
  BOOST_FOREACH( const Point& p, corners_ )
  {
    p2 += tm*p;
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
      BOOST_FOREACH( double& g, *gl )
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

/*
 def register(self, bmd):
     for c in self.corners:
         bmd.addPoint(Point(c))

 def face(self, i):
     if (i=='0321') or (i==0) or (i=='A') or (i=='a'):
         return [self.corners[0], self.corners[3], 
                 self.corners[2], self.corners[1]]
     elif (i=='2376') or (i==1) or (i=='B') or (i=='b'):
         return [self.corners[2], self.corners[3], 
                 self.corners[7], self.corners[6]]
     elif (i=='4567') or (i==2) or (i=='C') or (i=='c'):
         return [self.corners[4], self.corners[5], 
                 self.corners[6], self.corners[7]]
     elif (i=='0154') or (i==3) or (i=='D') or (i=='d'):
         return [self.corners[0], self.corners[1], 
                 self.corners[5], self.corners[4]]
     elif (i=='0473') or (i==4) or (i=='E') or (i=='e'):
         return [self.corners[0], self.corners[4], 
                 self.corners[7], self.corners[3]]
     elif (i=='1265') or (i==5) or (i=='F') or (i=='f'):
         return [self.corners[1], self.corners[2], 
                 self.corners[6], self.corners[5]]
     else:
         raise RuntimeError('Unknown face of block: '+str(i))
     

 def swapGrad(self):
     try:
         if len(self.grading[2])>0:
             for i in range(0,len(self.grading[2])):
                 self.grading[2][i]=1./self.grading[2][i]
     except:
         self.grading[2]=1./self.grading[2]

     try:
         if len(self.grading[0])==4:
             tmp0=self.grading[0][0]
             tmp1=self.grading[0][1]
             self.grading[0][0]=self.grading[0][3]
             self.grading[0][1]=self.grading[0][2]
             self.grading[0][3]=tmp0
             self.grading[0][2]=tmp1
     except:
         pass

     try:
         if len(self.grading[1])==4:
             tmp0=self.grading[1][0]
             tmp1=self.grading[1][1]
             self.grading[1][0]=self.grading[1][3]
             self.grading[1][1]=self.grading[1][2]
             self.grading[1][3]=tmp0
             self.grading[1][2]=tmp1
     except:
         pass

 def bmdEntry(self, allPoints):
     s="hex ( "
     #for c in self.corners:
     ci=range(0, 8)
     if (self.inv):
         ci=[4, 5, 6, 7, 0, 1, 2, 3]
     for i in ci:
         s+=str(allPoints[Point(self.corners[i])])+" "
     s+=") %s (%d %d %d) edgeGrading (" \
         % (self.zone,
            self.resolution[0], self.resolution[1], self.resolution[2]
            )

     for g in self.grading:
         try:
             if (len(g)==2):
                 s+="%f %f %f %f "%(g[0], g[1], g[1], g[0])
             elif (len(g)==4):
                 s+="%f %f %f %f "%(g[0], g[1], g[2], g[3])
         except:
             for i in range(0,4):
                 s+="%f "%g
     s+=")\n"
     return s
*/
std::vector<OFDictData::data>
Block::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> retval;
  retval.push_back( OFDictData::data("hex") );
  
  OFDictData::list cl;
  std::vector<int> ci;
  if (!inv_) ci+=0,1,2,3,4,5,6,7; else ci+=4,5,6,7,0,1,2,3;
  for (std::vector<int>::const_iterator i=ci.begin(); i!=ci.end(); i++)
  {
    cl.push_back( OFDictData::data(allPoints.find(corners_[*i])->second) );
  }
  retval.push_back( cl );
  
  retval.push_back( OFDictData::data(zone_) );

  OFDictData::list rl;
  rl += resolution_[0], resolution_[1], resolution_[2];
  retval.push_back( rl );

  OFDictData::list gl;
  for (GradingList::const_iterator g=grading_.begin(); g!=grading_.end(); g++)
  {
    try
    {
        double grad = boost::get<double>(*g) ;
        gl += grad, grad, grad, grad;
    }
    catch( const boost::bad_get& bg )
    {
        const std::vector<double>& gradl = boost::get<std::vector<double> >(*g);
	if (gradl.size()==2)
	  gl += gradl[0], gradl[1], gradl[1], gradl[0];
	else if (gradl.size()==4)
	  gl += gradl[0], gradl[1], gradl[2], gradl[3];
	else
	{
	  throw Exception("Invalid number of grading coefficients given!");
	}
    }
  }
  retval.push_back( gl );

  return retval;
}



transform2D::transform2D(int idx)
: idx_(idx),
  map_(),
  dir_()
{
    if (idx==0)
    {
	map_+=1,2; //<<1<<2; //[1, 2]
	dir_+=1,1,1; //<<1<<1<<1; //[1., 1., 1.]
    }
    else if (idx==1)
    {
	map_+=2,0; //<<2<<0; //[2, 0]
	dir_+=1,1,-1; //<<1<<1<<-1; //[1., 1., -1.]
    } 
    else if (idx==2)
    {
	map_+=0,1; //<<0<<1; //[0, 1]
	dir_+=1,1,1; //<<1<<1<<1; //[1., 1., 1.]
    }
}

mat transform2D::mapped3D(const mat& p) const
{
    mat p3=vec3(0., 0., 0.);
    p3[map_[0]]=dir_[0]*p[0];
    p3[map_[1]]=dir_[1]*p[1];
    return p3;
}
  

/*
    void addFwdRvsPatches(bmd)
    {
        bmd.addPatch("front", self.fwdPatch);
        bmd.addPatch("back", self.rvsPatch);
    }
    */



plane2D::plane2D(double thick, int idx)
: transform2D(idx)
{
  ofs_=vec3(0, 0, 0);
  ofs_[idx_]=0.5*thick*dir_[2];
  fwdPatch_=Patch("empty");
  rvsPatch_=Patch("empty");
}

mat plane2D::fwd(const mat& p) const
{
    return mapped3D(p)+ofs_;
}

mat plane2D::rvs(const mat& p) const
{
    return mapped3D(p)-ofs_;
}




wedge2D::wedge2D(int idx)
: transform2D(idx)
{
    mat ax=vec3(0., 0., 0.);
    ax[map_[0]]=1.0;
    fwdrot_=rotMatrix(2.5*M_PI/180., ax);
    rvsrot_=rotMatrix(-2.5*M_PI/180., ax);
    fwdPatch_=Patch("wedge");
    rvsPatch_=Patch("wedge");
}

mat wedge2D::fwd(const mat& p) const
{
    return fwdrot_*mapped3D(p);
}

mat wedge2D::rvs(const mat& p) const
{
    return rvsrot_*mapped3D(p);
}




Block2D::Block2D
(
  transform2D& t2d, 
  PointList corners, 
  int resx, int resy, 
  GradingList grading, 
  std::string zone, 
  bool inv
)
: Block
  (
    (boost::assign::list_of
	(t2d_.rvs(corners[0])), 
	(t2d_.rvs(corners[1])), 
	(t2d_.rvs(corners[2])), 
	(t2d_.rvs(corners[3])),
	(t2d_.fwd(corners[0])),
	(t2d_.fwd(corners[1])), 
	(t2d_.fwd(corners[2])),
	(t2d_.fwd(corners[3]))),
    resx, resy, 1,
    (boost::assign::list_of
	(grading[0]), (grading[1]), (1)),
	zone,
	inv
  ),

  t2d_(t2d)
	
{
  /*
    bmdBlock.__init__(self, [
	    t2d.rvs(corners[0]), t2d.rvs(corners[1]), 
	    t2d.rvs(corners[2]), t2d.rvs(corners[3]),
	    t2d.fwd(corners[0]), t2d.fwd(corners[1]), 
	    t2d.fwd(corners[2]), t2d.fwd(corners[3])],
		      [resolution[0], resolution[1], 1], 
		      [grading[0], grading[1], 1], zone, inv
		      )
		      */
    t2d_.fwdPatch().addFace
    (
      t2d_.fwd(corners[3]),
      t2d_.fwd(corners[2]), 
      t2d_.fwd(corners[1]),
      t2d_.fwd(corners[0])
    );
    
    t2d_.rvsPatch().addFace
    (
      t2d_.rvs(corners[0]),
      t2d_.rvs(corners[1]), 
      t2d_.rvs(corners[2]),
      t2d_.rvs(corners[3])
    );
}


Edge::Edge(const Point& c0, const Point& c1)
: c0_(c0), c1_(c1)
{
}

void Edge::registerPoints(blockMesh& bmd) const
{
  bmd.addPoint(c0_);
  bmd.addPoint(c1_);
}

ArcEdge::ArcEdge
(
  const Point& c0, 
  const Point& c1, 
  const Point& midpoint
)
: Edge(c0, c1),
  midpoint_(midpoint)
{
}

    
std::vector<OFDictData::data>
ArcEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> l;
  l.push_back( OFDictData::data("arc") );
  l.push_back( OFDictData::data(allPoints.find(c0_)->second) );
  l.push_back( OFDictData::data(allPoints.find(c1_)->second) );
  OFDictData::list pl;
  pl += OFDictData::data(midpoint_[0]), OFDictData::data(midpoint_[1]), OFDictData::data(midpoint_[2]);
  l.push_back(pl);
  return l;
}


Edge* ArcEdge::transformed(const arma::mat& tm) const
{
  return new ArcEdge(tm*c0_, tm*c1_, tm*midpoint_);
}



EllipseEdge::EllipseEdge
(
  const Point& c0, const Point& c1, 
  const Point& center,
  double ex
)
: Edge(c0, c1),
  center_(center),
  ex_(ex)
{
}

    
std::vector<OFDictData::data>
EllipseEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> l;
  l.push_back( OFDictData::data("arc") );
  l.push_back( OFDictData::data(allPoints.find(c0_)->second) );
  l.push_back( OFDictData::data(allPoints.find(c1_)->second) );
  OFDictData::list pl;
  pl += OFDictData::data(center_[0]), OFDictData::data(center_[1]), OFDictData::data(center_[2]);
  l.push_back(pl);
  l.push_back( OFDictData::data(ex_) );
  return l;
}

Edge* EllipseEdge::transformed(const arma::mat& tm) const
{
  throw insight::Exception("Not implemented!");
}



CircularEdge::CircularEdge
(
  const Point& c0, const Point& c1, 
  Point startPoint, 
  mat axis
)
: ArcEdge(c0, c1, vec3(0,0,0))
{
    mat bp = mat(startPoint) + axis*dot(axis, ( mat(c0_) - mat(startPoint) ));
    mat r1 = mat(c0_) - bp;
    double r = sqrt(dot(r1, r1));
    mat mp0 = 0.5*(( mat(c0_) - bp) + ( mat(c1_) - bp));
    mp0 /= sqrt(dot(mp0,mp0));
    midpoint_ = mat(bp) + r*mp0;
}


/*
class GenArcEdge
: public ArcEdge
{
    def __init__(self, corners, R, axis, startPoint=array([0,0,0])):
        from scipy.optimize.nonlin import broyden2
        a=axis
        a/=sqrt(dot(a,a))
        def F(x):
            M=array([x[0], x[1], x[2]])
            r1=corners[0]-M
            r2=corners[1]-M
            cr=cross(r1, r2)
            cr/=sqrt(dot(cr, cr))
            return [
                1.0-dot(a, cr),
                sqrt(dot(r1, r1))-R,
                sqrt(dot(r2, r2))-R
                 ]
        startPoint=array(
            broyden2(F, list(startPoint), 20, verbose=True)
            )
        print corners, startPoint
        bp=startPoint+axis*dot(axis, (corners[0]-startPoint))
        r1=corners[0]-bp
        r=sqrt(dot(r1, r1))
        print R, r
        mp0=0.5*((corners[0]-bp)+(corners[1]-bp))
        mp0/=sqrt(dot(mp0,mp0))
        bmdArcEdge.__init__(self, corners, bp+r*mp0)
};
*/

SplineEdge::SplineEdge(const PointList& points, string splinekeyword)
: Edge(points.front(), points.back()),
  intermediatepoints_(points.begin()+1, points.end()-1),
  splinekeyword_(splinekeyword)
{}

std::vector<OFDictData::data> SplineEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> l;
  //l.push_back( OFDictData::data(splinekeyword_) );
  if (OFversion<=160)
    l.push_back(splinekeyword_);
  else
    l.push_back("spline");
  l.push_back( OFDictData::data(allPoints.find(c0_)->second) );
  l.push_back( OFDictData::data(allPoints.find(c1_)->second) );
  
  OFDictData::list pl;
  BOOST_FOREACH( const Point& pt, intermediatepoints_ )
  {
    OFDictData::list ppl;
    ppl += OFDictData::data(pt[0]), OFDictData::data(pt[1]), OFDictData::data(pt[2]);
    pl.push_back(ppl);
  }
  l.push_back(pl);
  
  return l;
};

Edge* SplineEdge::transformed(const arma::mat& tm) const
{
  PointList pl;
  pl+=tm*c0_;
  BOOST_FOREACH(const Point& p, intermediatepoints_)
  {
    pl+=tm*p;
  }
  pl+=tm*c1_;
  return new SplineEdge(pl, splinekeyword_);
}

/*
def splineEdgeAlongCurve(curve, p0, p1, 
                         start0=0.5, start1=0.5, 
                         lim=False, fixed=False):
    if not fixed:
        t0=Curves.tNearP(curve, p0, start0, limited=lim)
        t1=Curves.tNearP(curve, p1, start1, limited=lim)
    else:
        t0=start0
        t1=start1
    print "Generating spline on curve from param ", t0, " to ", t1
    if (fabs(t0-t1)<1e-4):
        raise Exception('start and end of spline are the same point')
    allEdges.append(bmdSplineEdge([p0, p1], pointsInRange(curve, t0, t1)))
    return t0, t1
*/



Patch::Patch(std::string typ)
: typ_(typ)
{
}

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

std::vector<OFDictData::data> 
Patch::bmdEntry(const PointMap& allPoints, const std::string& name, int OFversion) const
{
  std::vector<OFDictData::data> retval;  
  
  if (OFversion<210)
  {
    ostringstream oss;
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


GradingAnalyzer::GradingAnalyzer(double grad)
: grad_(grad)
{
}

GradingAnalyzer::GradingAnalyzer(double delta0, double L, int n)
{
  struct Obj: public Objective1D
  {
    double Lbydelta0;
    int n;
    double F(double R) const
    { return (pow(R, n/(n-1.))-1.) / (pow(R, 1./(n-1.))-1.); }
    virtual double operator()(double x) const { return Lbydelta0-F(x); }
  } obj;
  obj.n=n;
  obj.Lbydelta0=L/delta0;
  grad_=nonlinearSolve1D(obj, 1.0001, 10000);
}

#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>

typedef boost::tuple<const GradingAnalyzer*,double,double> f_calc_n_param;

double f_calc_n(double n, void* ga)
{
  f_calc_n_param* p = static_cast<f_calc_n_param*>(ga);
  double L=p->get<1>();
  double delta0=p->get<2>();
  
  double G=pow(p->get<0>()->grad(), 1./(n-1.));
  int n_c=1+log( 1./G + L*(G-1.)/delta0)/log(G);
  
  cout << L<< " "<<delta0 << " "<<n<<" "<<G<<" "<<n_c<<endl;
  return n_c-n;
}

int GradingAnalyzer::calc_n(double delta0, double L) const
{
  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;

 
    workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);
 
    f.function = &f_calc_n;
    f_calc_n_param p(this, L, delta0);
    f.params = static_cast<void*>(&p);
 
    x_l = 2;
    x_r = 10000;
 
    gsl_root_fsolver_set(workspace_f, &f, x_l, x_r);
 
    for(times = 0; times < 100; times++)
    {
        status = gsl_root_fsolver_iterate(workspace_f);
 
        x_l = gsl_root_fsolver_x_lower(workspace_f);
        x_r = gsl_root_fsolver_x_upper(workspace_f);
 
        status = gsl_root_test_interval(x_l, x_r, 1.0e-13, 1.0e-20);
        if(status != GSL_CONTINUE)
        {
            break;
        }
    }
 
    gsl_root_fsolver_free(workspace_f);
    
  return x_l;
}

Patch2D::Patch2D(const transform2D& t2d, std::string typ)
: Patch(typ),
  t2d_(t2d)
{
}

void Patch2D::addFace(const PointList& corners)
{
    Patch::addFace
    (
      t2d_.fwd(corners[0]), 
      t2d_.fwd(corners[1]),
      t2d_.rvs(corners[1]), 
      t2d_.rvs(corners[0])
    );
}


/*
class ArcEdge2D
{
    def __init__(self, t2d, corners, midpoint):
        self.t2d=t2d
        self.corners=corners
        self.midpoint=midpoint

    def register(self, bmd):
        for c in self.corners:
            bmd.addPoint(Point(self.t2d.fwd(c)))
            bmd.addPoint(Point(self.t2d.rvs(c)))

    def bmdEntry(self, allPoints):
        mp=self.t2d.fwd(self.midpoint)
        s="arc %d %d (%f %f %f)\n" % (
            allPoints[Point(self.t2d.fwd(self.corners[0]))], 
            allPoints[Point(self.t2d.fwd(self.corners[1]))],
            mp[0], mp[1], mp[2])
        mp=self.t2d.rvs(self.midpoint)
        s+="arc %d %d (%f %f %f)\n" % (
            allPoints[Point(self.t2d.rvs(self.corners[0]))], 
            allPoints[Point(self.t2d.rvs(self.corners[1]))],
            mp[0], mp[1], mp[2])
        return s
};

class SplineEdge2D
{
    def __init__(self, t2d, corners, intermediatepoints, 
                 splinekeyword="GSLSpline"):
        self.t2d=t2d
        self.corners=corners
        self.intermediatepoints=intermediatepoints
        self.skey=splinekeyword

    def register(self, bmd):
        for c in self.corners:
            bmd.addPoint(Point(self.t2d.fwd(c)))
            bmd.addPoint(Point(self.t2d.rvs(c)))
            
    def bmdEntry(self, allPoints):
        s="%s %d %d\n(\n" % (
            self.skey,
            allPoints[Point(self.t2d.fwd(self.corners[0]))], 
            allPoints[Point(self.t2d.fwd(self.corners[1]))])
        for p in self.intermediatepoints:
            s+="(%s)\n"%(" ".join(["%g"%c for c in self.t2d.fwd(p)]))
        s+=")\n"

        s+="%s %d %d\n(\n" % (
            self.skey,
            allPoints[Point(self.t2d.rvs(self.corners[0]))], 
            allPoints[Point(self.t2d.rvs(self.corners[1]))])
        for p in self.intermediatepoints:
            s+="(%s)\n"%(" ".join(["%g"%c for c in self.t2d.rvs(p)]))
        s+=")\n"
        
        return s
};
*/

blockMesh::blockMesh(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "blockMesh"),
  scaleFactor_(1.0),
  defaultPatchName_("defaultFaces"),
  defaultPatchType_("wall"),
  allPoints_(compare)
{
}

void blockMesh::setScaleFactor(double sf)
{
  scaleFactor_=sf;
}

void blockMesh::setDefaultPatch(const std::string& name, std::string type)
{
  defaultPatchName_=name;
  defaultPatchType_=type;
}

void blockMesh::numberVertices(PointMap& pts) const
{
  int idx=0;
  for (PointMap::iterator i=pts.begin();
       i!=pts.end(); i++)
       {
	 i->second = idx++;
       }
}

void blockMesh::addIntoDictionaries(insight::OFdicts& dictionaries) const
{
  PointMap pts(allPoints_);
  numberVertices(pts);
  
  OFDictData::dict& blockMeshDict=dictionaries.addDictionaryIfNonexistent("constant/polyMesh/blockMeshDict");
  blockMeshDict["convertToMeters"]=scaleFactor_;
  
  OFDictData::dict def;
  def["name"]=OFDictData::data(defaultPatchName_);
  def["type"]=OFDictData::data(defaultPatchType_);
  blockMeshDict["defaultPatch"]=def;
  
  OFDictData::list vl;
  for (PointMap::const_iterator i=allPoints_.begin();
       i!=allPoints_.end(); i++)
       {
	 OFDictData::list cl;
	 cl += i->first[0], i->first[1], i->first[2];
	 vl.push_back( cl );
       }
  blockMeshDict["vertices"]=vl;
  
  int n_cells=0;
  OFDictData::list bl;
  for (boost::ptr_vector<Block>::const_iterator i=allBlocks_.begin();
       i!=allBlocks_.end(); i++)
       {
	 n_cells+=i->nCells();
	 std::vector<OFDictData::data> l = i->bmdEntry(pts, OFversion());
	 bl.insert( bl.end(), l.begin(), l.end() );
       }
  blockMeshDict["blocks"]=bl;
  cout<<"blockMeshDict will create "<<n_cells<<" cells."<<endl;

  OFDictData::list el;
  for (boost::ptr_vector<Edge>::const_iterator i=allEdges_.begin();
       i!=allEdges_.end(); i++)
       {
	 std::vector<OFDictData::data> l = i->bmdEntry(pts, OFversion());
	 el.insert( el.end(), l.begin(), l.end() );
       }
  blockMeshDict["edges"]=el;

  OFDictData::list pl;
  for (boost::ptr_map<std::string, Patch>::const_iterator i=allPatches_.begin();
       i!=allPatches_.end(); i++)
       {
	 std::vector<OFDictData::data> l = i->second->bmdEntry(pts, i->first, OFversion());
	 pl.insert( pl.end(), l.begin(), l.end() );
       }
  if (OFversion()<210)
    blockMeshDict["patches"]=pl;
  else
    blockMeshDict["boundary"]=pl;

  OFDictData::list mppl;
  blockMeshDict["mergePatchPairs"]=mppl;

}

}

}