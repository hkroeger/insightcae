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

#include "geotest.h"

#include <memory>
#include "base/linearalgebra.h"
#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include "boost/lexical_cast.hpp"

#include "featurefilter.h"
#include "solidmodel.h"

#include "gp_Cylinder.hxx"

#define BOOST_SPIRIT_USE_PHOENIX_V3
// #define BOOST_SPIRIT_DEBUG

#include "base/boost_include.h"
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include "datum.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


std::ostream& operator<<(std::ostream& os, const FeatureSet& fs)
{
  os<<fs.size()<<" {";
  BOOST_FOREACH(int fi, fs)
  {
    os<<" "<<fi;
  }
  os<<" }";
  return os;
}

Filter::Filter()
: model_(NULL)
{
}

Filter::~Filter()
{
}

void Filter::initialize(const SolidModel& m)
{
  model_=&m;
}

void Filter::firstPass(FeatureID feature)
{
}


AND::AND(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}

void AND::initialize(const SolidModel& m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

void AND::firstPass(FeatureID feature)
{
  Filter::firstPass(feature);
  f1_->firstPass(feature);
  f2_->firstPass(feature);
}


bool AND::checkMatch(FeatureID feature) const
{
  return f1_->checkMatch(feature) && f2_->checkMatch(feature);
}

FilterPtr AND::clone() const
{
  return FilterPtr(new AND(*f1_, *f2_));
}



OR::OR(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}

void OR::initialize(const SolidModel& m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

void OR::firstPass(FeatureID feature)
{
  Filter::firstPass(feature);
  f1_->firstPass(feature);
  f2_->firstPass(feature);
}

bool OR::checkMatch(FeatureID feature) const
{
//   cout<<feature<<" -> "<<f1_->checkMatch(feature) <<"||"<< f2_->checkMatch(feature)<<endl;
  return f1_->checkMatch(feature) || f2_->checkMatch(feature);
}

FilterPtr OR::clone() const
{
  return FilterPtr(new OR(*f1_, *f2_));
}



NOT::NOT(const Filter& f1)
: Filter(),
  f1_(f1.clone())
{
}
void NOT::initialize(const SolidModel& m)
{
  f1_->initialize(m);
}

bool NOT::checkMatch(FeatureID feature) const
{
  return !f1_->checkMatch(feature);
}

FilterPtr NOT::clone() const
{
  return FilterPtr(new NOT(*f1_));
}

FilterPtr Filter::operator&&(const Filter& f2)
{
  return FilterPtr(new AND(*this, f2));
}

FilterPtr Filter::operator!()
{
  return FilterPtr(new NOT(*this));
}

// ANDFilter operator&&(const Filter& f1, const Filter& f2)
// {
//   return ANDFilter(f1, f2);
// }
// 
// NOTFilter operator!(const Filter& f1)
// {
//   return NOTFilter(f1);
// }

boundaryEdge::boundaryEdge()
{
}

void boundaryEdge::initialize(const SolidModel& m)
{
    insight::cad::Filter::initialize(m);
    safb_.reset(new ShapeAnalysis_FreeBounds(m));
}


bool boundaryEdge::checkMatch(FeatureID feature) const
{
  for (TopExp_Explorer ex(safb_->GetOpenWires(), TopAbs_EDGE); ex.More(); ex.Next())
  {
    if (TopoDS::Edge(ex.Current()).IsEqual(model_->edge(feature)))
      return true;
  }
  for (TopExp_Explorer ex(safb_->GetClosedWires(), TopAbs_EDGE); ex.More(); ex.Next())
  {
    if (TopoDS::Edge(ex.Current()).IsEqual(model_->edge(feature)))
      return true;
  }
  return false;
}

FilterPtr boundaryEdge::clone() const
{
  return FilterPtr(new boundaryEdge());  
}


edgeTopology::edgeTopology(GeomAbs_CurveType ct)
: ct_(ct)
{
}

bool edgeTopology::checkMatch(FeatureID feature) const
{
  return model_->edgeType(feature) == ct_;
}

FilterPtr edgeTopology::clone() const
{
  return FilterPtr(new edgeTopology(ct_));
}

faceTopology::faceTopology(GeomAbs_SurfaceType ct)
: ct_(ct)
{
}

bool faceTopology::checkMatch(FeatureID feature) const
{
  return model_->faceType(feature) == ct_;
}

FilterPtr faceTopology::clone() const
{
  return FilterPtr(new faceTopology(ct_));
}

in::in(FeatureSet set)
: set_(set)
{
}

bool in::checkMatch(FeatureID feature) const
{
  return set_.find(feature)!=set_.end();
}

FilterPtr in::clone() const
{
  return FilterPtr(new in(set_));
}



faceAdjacentToEdges::faceAdjacentToEdges(FeatureSet edges)
: edges_(edges)
{
}

bool faceAdjacentToEdges::checkMatch(FeatureID feature) const
{
  TopoDS_Face f=model_->face(feature);
  for(TopExp_Explorer ex(f, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge e=TopoDS::Edge(ex.Current());
    BOOST_FOREACH(FeatureID ei, edges_)
    {
      TopoDS_Edge e2=edges_.model().edge(ei);
      if (e.IsSame(e2))
	return true;
    }
  }
  return false;
}

FilterPtr faceAdjacentToEdges::clone() const
{
  return FilterPtr(new faceAdjacentToEdges(edges_));
}


faceAdjacentToFaces::faceAdjacentToFaces(FeatureSet faces)
: faces_(faces)
{
}

bool faceAdjacentToFaces::checkMatch(FeatureID feature) const
{
  TopoDS_Face f=model_->face(feature);
  for(TopExp_Explorer ex(f, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge e=TopoDS::Edge(ex.Current());
    BOOST_FOREACH(FeatureID fi, faces_)
    {
      bool valid=true;
      if (*model_==faces_.model())
	if (fi==feature) valid=false;
	
      if (valid)
      {
	TopoDS_Face f2=faces_.model().face(fi);
	for(TopExp_Explorer ex2(f2, TopAbs_EDGE); ex2.More(); ex2.Next())
	{
	  TopoDS_Edge e2=TopoDS::Edge(ex2.Current());
	  if (e.IsSame(e2))
	    return true;
	}
      }
    }
  }
  return false;
}

FilterPtr faceAdjacentToFaces::clone() const
{
  return FilterPtr(new faceAdjacentToFaces(faces_));
}

cylFaceOrientation::cylFaceOrientation(bool io)
: io_(io)
{
}

bool cylFaceOrientation::checkMatch(FeatureID feature) const
{
  if (model_->faceType(feature)==GeomAbs_Cylinder)
  {
      GeomAdaptor_Surface adapt(BRep_Tool::Surface(model_->face(feature)));
      gp_Cylinder icyl=adapt.Cylinder();
      gp_Ax1 iax=icyl.Axis();
      BRepGProp_Face prop(model_->face(feature));
      double u1,u2,v1,v2;
      prop.Bounds(u1, u2, v1, v2);
      double u = (u1+u2)/2;
      double v = (v1+v2)/2;
      gp_Vec vec;
      gp_Pnt pnt;
      prop.Normal(u,v,pnt,vec);
      vec.Normalize();
      gp_XYZ dp=pnt.XYZ()-icyl.Location().XYZ();
      gp_XYZ ax=iax.Direction().XYZ();
      ax.Normalize();
      gp_XYZ dr=dp-ax.Multiplied(dp.Dot(ax));
      dr.Normalize();
      
      if (io_)
      {
	if (! (fabs(vec.XYZ().Dot(dr) + 1.) < 1e-6) ) return true;
      }
      else
      {
	if (! (fabs(vec.XYZ().Dot(dr) - 1.) < 1e-6) ) return true;
      }
  }
  else
    return false;
}

FilterPtr cylFaceOrientation::clone() const
{
  return FilterPtr(new cylFaceOrientation(io_));
}

everything::everything()
{}

bool everything::checkMatch(FeatureID feature) const
{
  return true;
}
  
FilterPtr everything::clone() const
{
  return FilterPtr(new everything());
}

vertexLocation::vertexLocation() 
{}

vertexLocation::~vertexLocation()
{}
  
arma::mat vertexLocation::evaluate(FeatureID vi)
{
  return model_->vertexLocation(vi);
}
  
QuantityComputer<arma::mat>::Ptr vertexLocation::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new vertexLocation());
}


edgeCoG::edgeCoG() 
{}

edgeCoG::~edgeCoG()
{}
  
arma::mat edgeCoG::evaluate(FeatureID ei)
{
  return model_->edgeCoG(ei);
}
  
QuantityComputer<arma::mat>::Ptr edgeCoG::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new edgeCoG());
}

edgeMidTangent::edgeMidTangent() 
{}

edgeMidTangent::~edgeMidTangent()
{}
  
arma::mat edgeMidTangent::evaluate(FeatureID ei)
{
// #error Implement me!
//   return model_->edgeMidTangent(ei);
  return vec3(0,0,0);
}
  
QuantityComputer<arma::mat>::Ptr edgeMidTangent::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new edgeMidTangent());
}

edgeStart::edgeStart() 
{}

edgeStart::~edgeStart()
{}
  
arma::mat edgeStart::evaluate(FeatureID ei)
{
  gp_Pnt p=BRep_Tool::Pnt(TopExp::FirstVertex(model_->edge(ei)));
  return Vector(p);
}
  
QuantityComputer<arma::mat>::Ptr edgeStart::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new edgeStart());
}

edgeEnd::edgeEnd() 
{}

edgeEnd::~edgeEnd()
{}
  
arma::mat edgeEnd::evaluate(FeatureID ei)
{
  gp_Pnt p=BRep_Tool::Pnt(TopExp::LastVertex(model_->edge(ei)));
  return Vector(p);
}
  
QuantityComputer<arma::mat>::Ptr edgeEnd::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new edgeEnd());
}

edgeLen::edgeLen()
{
}

edgeLen::~edgeLen()
{
}

double edgeLen::evaluate(FeatureID ei)
{
  GProp_GProps gpr;
  TopoDS_Edge e=model_->edge(ei);
  double l = 0.;
  if (!e.IsNull() && !BRep_Tool::Degenerated(e)) {
    BRepGProp::LinearProperties(e, gpr);
    l = gpr.Mass();
  }
  return l;
}

QuantityComputer< double >::Ptr edgeLen::clone() const
{
  return QuantityComputer<double>::Ptr(new edgeLen());
}



faceArea::faceArea()
{
}

faceArea::~faceArea()
{
}

double faceArea::evaluate(FeatureID ei)
{
  GProp_GProps gpr;
  TopoDS_Face e=model_->face(ei);
  double l = 0.;
  if (!e.IsNull() /*&& !BRep_Tool::Degenerated(e)*/) {
    BRepGProp::SurfaceProperties(e, gpr);
    l = gpr.Mass();
  }
  return l;
}

QuantityComputer< double >::Ptr faceArea::clone() const
{
  return QuantityComputer<double>::Ptr(new faceArea());
}


edgeRadialLen::edgeRadialLen(const boost::shared_ptr<matQuantityComputer >& ax, const boost::shared_ptr<matQuantityComputer >& p0)
: ax_(ax), p0_(p0)
{
}

edgeRadialLen::~edgeRadialLen()
{
}

double edgeRadialLen::evaluate(FeatureID ei)
{
  arma::mat ax=ax_->evaluate(ei);
  arma::mat p0=p0_->evaluate(ei);
  
  arma::mat p1=(Vector(BRep_Tool::Pnt(TopExp::FirstVertex(model_->edge(ei))))).t()-p0;
  arma::mat p2=(Vector(BRep_Tool::Pnt(TopExp::LastVertex(model_->edge(ei))))).t()-p0;
  
  p1-=ax*(dot(ax, p1));
  p2-=ax*(dot(ax, p2));
  
  double rl=fabs(norm(p1,2)-norm(p2,2));
  std::cout<<"edge "<<ei<<": rl="<<rl<<std::endl;
  return rl;
}

QuantityComputer< double >::Ptr edgeRadialLen::clone() const
{
  return QuantityComputer<double>::Ptr(new edgeRadialLen(ax_, p0_));
}


faceCoG::faceCoG() 
{}

faceCoG::~faceCoG()
{}
  
arma::mat faceCoG::evaluate(FeatureID ei)
{
  return model_->faceCoG(ei);
}
  
QuantityComputer<arma::mat>::Ptr faceCoG::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new faceCoG());
}


solidCoG::solidCoG() 
{}

solidCoG::~solidCoG()
{}
  
arma::mat solidCoG::evaluate(FeatureID i)
{
  return model_->subsolidCoG(i);
}
  
QuantityComputer<arma::mat>::Ptr solidCoG::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new solidCoG());
}


faceNormalVector::faceNormalVector() 
{}

faceNormalVector::~faceNormalVector()
{}
  
arma::mat faceNormalVector::evaluate(FeatureID fi)
{
  return model_->faceNormal(fi);
}
  
QuantityComputer<arma::mat>::Ptr faceNormalVector::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new faceNormalVector());
}
  
  
cylRadius::cylRadius() 
{}

cylRadius::~cylRadius()
{}

bool cylRadius::isValidForFeature(FeatureID f) const
{
  return model_->faceType(f) == GeomAbs_Cylinder;
}
  
double cylRadius::evaluate(FeatureID fi)
{
  if (model_->faceType(fi)==GeomAbs_Cylinder)
  {
      GeomAdaptor_Surface adapt(BRep_Tool::Surface(model_->face(fi)));
      gp_Cylinder icyl=adapt.Cylinder();
      return icyl.Radius();
  }
  else return -1.0;
}
  
QuantityComputer<double>::Ptr cylRadius::clone() const 
{
  return QuantityComputer<double>::Ptr(new cylRadius());
}


template<> coincident<Edge>::coincident(const SolidModel& m)
: f_(m.allEdges())
{
}

template<>
bool coincident<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_)
  {
    TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
    TopoDS_Edge e2=TopoDS::Edge(f_.model().edge(f));
    match |= isPartOf(e2, e1);
  }
  
  return match;
}

template<> coincident<Face>::coincident(const SolidModel& m)
: f_(m.allFaces())
{
}

template<>
bool coincident<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_)
  {
    TopoDS_Face e1=TopoDS::Face(model_->face(feature));
    TopoDS_Face e2=TopoDS::Face(f_.model().face(f));
    match |= isPartOf(e2, e1);
  }
  
  return match;
}

coincidentProjectedEdge::coincidentProjectedEdge
(
  const SolidModel& m, 
  const matQuantityComputerPtr& p0, 
  const matQuantityComputerPtr& n, 
  const matQuantityComputerPtr& up,
  const scalarQuantityComputerPtr& tol
)
: f_(m, Edge),
  p0_(p0), n_(n), up_(up), tol_(tol)
{
}

coincidentProjectedEdge::coincidentProjectedEdge
(
  FeatureSet f, 
  const matQuantityComputerPtr& p0, 
  const matQuantityComputerPtr& n, 
  const matQuantityComputerPtr& up,
  const scalarQuantityComputerPtr& tol
)
: f_(f),
  p0_(p0), n_(n), up_(up), tol_(tol)
{
}

void coincidentProjectedEdge::firstPass(FeatureID feature)
{
  if (samplePts_.n_cols==0)
  {
    double tol=tol_->evaluate(feature);
    
    BOOST_FOREACH(int f, f_)
    {    
      TopoDS_Edge e=TopoDS::Edge(f_.model().edge(f));
      BRepAdaptor_Curve ac(e);
      GCPnts_QuasiUniformDeflection qud(ac, 0.1*tol);

      arma::mat mypts=arma::zeros<arma::mat>(3,qud.NbPoints());
      for (int j=1; j<=qud.NbPoints(); j++)
      {
	arma::mat p=Vector(qud.Value(j)).t();
	mypts.col(j-1)=p;
      }
      
      if (samplePts_.n_cols==0)
	samplePts_=mypts;
      else
	samplePts_=join_rows(samplePts_, mypts);
      
//       cout <<"f="<<f<<", #cols="<<samplePts_.n_cols<<endl;
    }
    
//     arma::mat spt=samplePts_.t();
//     spt.save("samplePts.csv", arma::raw_ascii);
  }
}


bool coincidentProjectedEdge::checkMatch(FeatureID feature) const
{
  DatumPlane pln(p0_->evaluate(feature), n_->evaluate(feature), up_->evaluate(feature));
  double tol=tol_->evaluate(feature);
  
  TopoDS_Edge e1=model_->edge(feature);
  SolidModel se1(e1);
  ProjectedOutline po(se1, pln);
  TopoDS_Shape pe=po;
  
//   if (!dbgfile_)
//   {
//     dbgfile_.reset(new std::ofstream("check.csv"));
//   }
  
  bool match=true;
  for (TopExp_Explorer ex(pe, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge e=TopoDS::Edge(ex.Current());
    BRepAdaptor_Curve ac(e);
    GCPnts_QuasiUniformDeflection qud(ac, tol);

    for (int j=1; j<=qud.NbPoints(); j++)
    {
      arma::mat p=Vector(qud.Value(j)).t();
//       (*dbgfile_) << p(0) << " " << p(1) << " "<<p(2)<<endl;
      // find closest sample pt
      arma::mat d=samplePts_ - p*arma::ones<arma::mat>(1,samplePts_.n_cols);
      arma::mat ds=sqrt( d.row(0)%d.row(0) + d.row(1)%d.row(1) + d.row(2)%d.row(2) );
//       cout<<"ds="<<ds<<endl;
//       cout<<"min(ds)="<<arma::min(ds,1)<<endl;
      double md=arma::as_scalar(arma::min(ds, 1));
//       cout <<md<<" => "<<(samplePts_ - p*arma::ones<arma::mat>(1,samplePts_.n_cols))<<endl;
      match=match & (md<tol);
    }
  }
  
//   (*dbgfile_)<<endl<<endl;
  
  return match;
}

FilterPtr coincidentProjectedEdge::clone() const
{
  return FilterPtr(new coincidentProjectedEdge(f_, p0_, n_, up_, tol_));
}


template<> isPartOfSolid<Edge>::isPartOfSolid(const SolidModel& m)
: s_(TopoDS::Solid(m))
{
}

template<>
bool isPartOfSolid<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
  match |= isPartOf(s_, e1);
  
  return match;
}

template<> isPartOfSolid<Face>::isPartOfSolid(const SolidModel& m)
: s_(TopoDS::Solid(m))
{
}

template<>
bool isPartOfSolid<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
//   BOOST_FOREACH(int f, f_)
  {
    TopoDS_Face e1=TopoDS::Face(model_->face(feature));
    match |= isPartOf(s_, e1);
  }
  
  return match;
}



template<>
bool secant<Edge>::checkMatch(FeatureID feature) const
{
  TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));

  TopoDS_Vertex v0=TopExp::FirstVertex(e1);
  TopoDS_Vertex v1=TopExp::LastVertex(e1);
  arma::mat v = Vector( BRep_Tool::Pnt(v0).XYZ() - BRep_Tool::Pnt(v1).XYZ() );
  
  return (1.0 - fabs(arma::dot( v/arma::norm(v,2), dir_/arma::norm(dir_,2) ))) < 1e-10;
}



maximal::maximal(const scalarQuantityComputer& qtc, int rank)
: qtc_(qtc.clone()),
  rank_(rank)
{
}

void maximal::initialize(const SolidModel& m)
{
  Filter::initialize(m);
  qtc_->initialize(m);
}

void maximal::firstPass(FeatureID feature)
{
  if (qtc_->isValidForFeature(feature))
    ranking_[-qtc_->evaluate(feature)].insert(feature);
}

bool maximal::checkMatch(FeatureID feature) const
{
  int j=0;
  for (std::map<double, std::set<FeatureID> >::const_iterator i=ranking_.begin(); i!=ranking_.end(); i++)
  {
    if ((i->second.find(feature)!=i->second.end()) && (j==rank_)) return true;
    j++;
  }
  return false;
}

FilterPtr maximal::clone() const
{
  return FilterPtr(new maximal(*qtc_, rank_));
}

minimal::minimal(const scalarQuantityComputer& qtc, int rank)
: maximal(qtc, rank)
{}

void minimal::firstPass(FeatureID feature)
{
  if (qtc_->isValidForFeature(feature))
    ranking_[qtc_->evaluate(feature)].insert(feature);
}

FilterPtr minimal::clone() const
{
  return FilterPtr(new minimal(*qtc_, rank_));
}


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;
using namespace insight::cad;


FeatureSetPtr lookupFeatureSet(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "Feature set #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  
  if (FeatureSetPtr* fsp = boost::get<FeatureSetPtr>(&fl.at(id)))
  {
    return FeatureSetPtr( (*fsp)->clone() );
  }
  else
  {
    throw insight::Exception
    (
      "Argument #"+lexical_cast<std::string>(id)+" is not a FeatureSet"
    );
  }
  return FeatureSetPtr();
}
BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, lookupFeatureSet_, lookupFeatureSet, 2);

arma::mat lookupMat(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "Vector entry #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  
  if (arma::mat* m = boost::get<arma::mat>(&fl.at(id)))
  {
    return *m;
  }
  else
  {
    throw insight::Exception
    (
      "Argument #"+lexical_cast<std::string>(id)+" is not a vector/matrix"
    );
  }
  return arma::mat();
}

double lookupScalar(const FeatureSetParserArgList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "scalar entry #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  
  if (double* m = boost::get<double>(&fl.at(id)))
  {
    return *m;
  }
  else
  {
    throw insight::Exception
    (
      "Argument #"+lexical_cast<std::string>(id)+" is not a scalar"
    );
  }
  return 0.0;
}
BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec3_, vec3, 3);

template <typename Iterator, typename Skipper = qi::space_type >
struct FeatureFilterExprParser
: qi::grammar<Iterator, FilterPtr(), Skipper>
{

public:
    qi::rule<Iterator, std::string(), Skipper> r_identifier;
    qi::rule<Iterator, FeatureSetPtr(), Skipper> r_featureset;
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter_primary, r_filter_and, r_filter_or;
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter;
    qi::rule<Iterator, FilterPtr(), Skipper> r_qty_comparison;
    qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_expression, r_scalar_primary, r_scalar_term;
    qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_expression, r_mat_primary, r_mat_term;
    
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter_functions;
    qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_functions;
    qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_functions;
    
    FeatureSetParserArgList externalFeatureSets_;

    FeatureFilterExprParser
    (
      const FeatureSetParserArgList& extsets
    )
    : FeatureFilterExprParser::base_type(r_filter),
      externalFeatureSets_(extsets)
    {
	r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
	r_featureset = lexeme[ '%' >> qi::int_ ] [ qi::_val = phx::construct<FeatureSetPtr>(lookupFeatureSet_(externalFeatureSets_, qi::_1)) ];

        r_filter =  r_filter_or.alias();

	r_filter_or = 
	  ( r_filter_and >> lit("||") > r_filter_and ) [ _val = phx::construct<FilterPtr>(new_<OR>(*qi::_1, *qi::_2)) ] 
	  | r_filter_and [ qi::_val = qi::_1 ]
	  ;
	
	r_filter_and =
	  ( r_filter_primary >> lit("&&") > r_filter_primary ) [ _val = phx::construct<FilterPtr>(new_<AND>(*qi::_1, *qi::_2)) ]
	  | r_filter_primary [ qi::_val = qi::_1 ]
	  ;
	  
	r_filter_primary =
	  ( r_filter_functions ) [ qi::_val = qi::_1 ]
	  |
	  ( lit("in") >> '(' > r_featureset > ')' ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<in>(*qi::_1)) ]
	  |
	  ( lit("maximal") >> '(' > r_scalar_qty_expression >> ( ( ',' > int_ ) | attr(0) ) >> ')' ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<maximal>(*qi::_1, qi::_2)) ]
	  |
	  ( lit("minimal") >> '(' > r_scalar_qty_expression >> ( ( ',' > int_ ) | attr(0) ) >> ')' ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<minimal>(*qi::_1, qi::_2)) ]
	  |
	  ( r_qty_comparison ) [ qi::_val = qi::_1 ]
	  |
	  ( '(' >> r_filter >> ')' ) [ qi::_val = qi::_1 ]
	  | 
	  ( '!' >> r_filter_primary ) [ qi::_val = phx::construct<FilterPtr>(new_<NOT>(*qi::_1)) ]
	  ;
	  
	r_qty_comparison = 
	  ( r_scalar_qty_expression >> lit("==") >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<equal<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> '~' >> r_scalar_qty_expression >> ( ( '{' >> double_ >> '}' ) | attr(1e-2) ) ) 
	    [ qi::_val = phx::construct<FilterPtr>(new_<approximatelyEqual<double> >(*qi::_1, *qi::_2, qi::_3)) ]
	  |
	  ( r_scalar_qty_expression >> '>' >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<greater<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> lit(">=") >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<greaterequal<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> '<' >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<less<double, double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_scalar_qty_expression >> lit("<=") >> r_scalar_qty_expression ) [ qi::_val = phx::construct<FilterPtr>(new_<lessequal<double, double> >(*qi::_1, *qi::_2)) ]
	  ;
	  
	r_scalar_qty_expression =
	  ( r_scalar_term >> '+' > r_scalar_term ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<added<double,double> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_scalar_term >> '-' > r_scalar_term ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<subtracted<double,double> >(*qi::_1, *qi::_2)) ]
	  |
	  r_scalar_term [ qi::_val = qi::_1 ]
	  ;
	
	r_scalar_term =
	  r_scalar_primary [ qi::_val = qi::_1 ]
	  |
	  ( r_scalar_primary >> '*' >> r_scalar_primary ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<multiplied<double,double> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_scalar_primary >> '/' >> r_scalar_primary ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<divided<double,double> >(*qi::_1, *qi::_2)) ]
	  |
	  ( r_mat_primary >> '&' >> r_mat_primary ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<dotted<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  ;
	  
	r_scalar_primary =
	  /*lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  |*/ double_ [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<constantQuantity<double> >(qi::_1)) ]
	  | r_scalar_qty_functions [ qi::_val = qi::_1 ]
	  | ( lit("mag") > '(' > r_scalar_qty_expression > ')' ) 
	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<mag<double> >(*qi::_1)) ]
	  | ( lit("dist") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<distance>(qi::_1, qi::_2)) ]
	  | ( lit("sqr") > '(' > r_scalar_qty_expression > ')' ) 
	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<sqr<double> >(*qi::_1)) ]
	  | ( lit("angle") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<angle<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  | ( lit("angleMag") > '(' > r_mat_qty_expression > ',' > r_mat_qty_expression > ')' ) 
	   [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<angleMag<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  |
	  lexeme[ lit("%d") >> qi::int_ ] [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>
	      ( new_<constantQuantity<double> >(phx::bind(&lookupScalar, externalFeatureSets_, qi::_1)) ) ]	  
	  | ( '(' >> r_scalar_qty_expression >> ')' ) [ qi::_val = qi::_1 ]
// 	  | ('-' >> r_scalar_primary) [ _val = -_1 ]
	  | ( r_mat_primary >> '.' >> 'x' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compX<arma::mat> >(*qi::_1)) ]
	  | ( r_mat_primary >> '.' >> 'y' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compY<arma::mat> >(*qi::_1)) ]
	  | ( r_mat_primary >> '.' >> 'z' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compZ<arma::mat> >(*qi::_1)) ]
	  ;
	  
	r_mat_qty_expression =
	  r_mat_term [ qi::_val = qi::_1 ]
	  |
	  ( r_mat_term >> '+' >> r_mat_term ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<added<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_mat_term >> '-' >> r_mat_term ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<subtracted<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  ;
	
	r_mat_term =
	(
	  r_mat_primary [ qi::_val = qi::_1 ]
	  |
	  ( r_mat_primary >> '*' >> r_mat_primary ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<multiplied<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	  | 
	  ( r_mat_primary >> '/' >> r_mat_primary ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<divided<arma::mat,arma::mat> >(*qi::_1, *qi::_2)) ]
	) /*| (
	  r_vector_primary >> '&' >> r_vector_primary
	) [_val = dot_(_1, _2) ]*/
	  ;
	  
	r_mat_primary =
	  /*lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  |*/ 
	  ( '[' >> double_ >> ',' >> double_ >> ',' >> double_ >> ']' ) 
	    [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<constantQuantity<arma::mat> >(vec3_(qi::_1,qi::_2,qi::_3))) ]
	  | 
	  r_mat_qty_functions [ qi::_val = qi::_1 ]
	  | 
	  ( '(' >> r_mat_qty_expression >> ')' ) [ qi::_val = qi::_1 ]
// 	  | ('-' >> r_scalar_primary) [ qi::_val = -qi::_1 ]
	  |
	  lexeme[ lit("%m") >> qi::int_ ] 
	   [ qi::_val = phx::construct<matQuantityComputer::Ptr>
	      ( new_<constantQuantity<arma::mat> >(phx::bind(&lookupMat, externalFeatureSets_, qi::_1)) ) ];	  
	  ;

//       BOOST_SPIRIT_DEBUG_NODE(r_filter);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_or);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_and);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_primary);
//       
//       BOOST_SPIRIT_DEBUG_NODE(r_qty_comparison);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_qty_expression);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);

      on_error<fail>(r_filter,
                       phx::ref(std::cout)
                       << "Error! Expecting "
                       << qi::_4
                       << " here: '"
                       << phx::construct<std::string>(qi::_3, qi::_2)
                       << "'\n"
                      );
    }  
    
    
};

template <typename Iterator/*, typename Skipper = skip_grammar<Iterator>*/ >
struct VertexFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  VertexFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
//     FeatureFilterExprParser<Iterator>::r_filter_functions =
// 	( lit("isPartOfSolid") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<isPartOfSolidEdge>(*qi::_1)) ]
// 	|
// 	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentEdge>(*qi::_1)) ]
// 	|
// 	( lit("projectionIsCoincident") > '('
// 	  > FeatureFilterExprParser<Iterator>::r_featureset > ','
// 	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ',' // p0
// 	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ',' // n
// 	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ','    // up
// 	  > FeatureFilterExprParser<Iterator>::r_scalar_qty_expression > ')' // tol
// 	) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentProjectedEdge>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
// 	  ;
	  
//       FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
// 	( lit("radius") >> 
// 	    '(' >> FeatureFilterExprParser<Iterator>::r_mat_qty_expression >> //ax
// 	    ',' >> FeatureFilterExprParser<Iterator>::r_mat_qty_expression >>  //p0
// 	    ')' ) 
// 	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<vertexRadius>(qi::_1, qi::_2)) ]
//       ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
//         ( lit("avgTangent") ) [ _val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeAvgTangent>()) ]
//         |
        ( lit("loc") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::vertexLocation>()) ]
      ;
    
  }
};


template <typename Iterator/*, typename Skipper = skip_grammar<Iterator>*/ >
struct EdgeFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  EdgeFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
    FeatureFilterExprParser<Iterator>::r_filter_functions =
	( lit("isLine") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Line)) ]
	|
	( lit("isCircle") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Circle)) ]
	|
	( lit("isEllipse") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Ellipse)) ]
	|
	( lit("isHyperbola") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Hyperbola)) ]
	|
	( lit("isParabola") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Parabola)) ]
	|
	( lit("isBezierCurve") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_BezierCurve)) ]
	|
	( lit("isBSplineCurve") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_BSplineCurve)) ]
	|
	( lit("isOtherCurve") ) [ qi::_val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_OtherCurve)) ]
	|
	( lit("isFaceBoundary") ) [ qi::_val = phx::construct<FilterPtr>(new_<boundaryEdge>()) ]
	|
	( lit("isPartOfSolid") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<isPartOfSolidEdge>(*qi::_1)) ]
	|
	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentEdge>(*qi::_1)) ]
	|
	( lit("projectionIsCoincident") > '('
	  > FeatureFilterExprParser<Iterator>::r_featureset > ','
	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ',' // p0
	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ',' // n
	  > FeatureFilterExprParser<Iterator>::r_mat_qty_expression > ','    // up
	  > FeatureFilterExprParser<Iterator>::r_scalar_qty_expression > ')' // tol
	) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentProjectedEdge>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
	  ;
	  
      FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
	( lit("len") ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<edgeLen>()) ]
	|
	( lit("radialLen") >> 
	    '(' >> FeatureFilterExprParser<Iterator>::r_mat_qty_expression >> //ax
	    ',' >> FeatureFilterExprParser<Iterator>::r_mat_qty_expression >>  //p0
	    ')' ) 
	  [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<edgeRadialLen>(qi::_1, qi::_2)) ]
      ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
//         ( lit("avgTangent") ) [ _val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeAvgTangent>()) ]
//         |
        ( lit("CoG") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeCoG>()) ]
         |
        ( lit("start") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeStart>()) ]
         |
        ( lit("end") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::edgeEnd>()) ]
      ;
    
  }
};


template <typename Iterator/*, typename Skipper = skip_grammar<Iterator>*/ >
struct FaceFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  FaceFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
    FeatureFilterExprParser<Iterator>::r_filter_functions = 
	( lit("isPlane") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Plane)) ]
	|
	( lit("isCylinder") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cylinder)) ]
	|
	( lit("isCone") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cone)) ]
	|
	( lit("isSphere") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Sphere)) ]
	|
	( lit("isTorus") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Torus)) ]
	|
	( lit("isBezierSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BezierSurface)) ]
	|
	( lit("isBSplineSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BSplineSurface)) ]
	|
	( lit("isSurfaceOfRevolution") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfRevolution)) ]
	|
	( lit("isSurfaceOfExtrusion") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfExtrusion)) ]
	|
	( lit("isOffsetSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OffsetSurface)) ]
	|
	( lit("isOtherSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OtherSurface)) ]
	|
	( lit("isPartOfSolid") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<isPartOfSolidFace>(*qi::_1)) ]
	|
	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentFace>(*qi::_1)) ]
	|
	( lit("adjacentToEdges") > '(' > FeatureFilterExprParser<Iterator>::r_featureset > ')' ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceAdjacentToEdges>(*qi::_1)) ]
	|
	( lit("adjacentToFaces") > '(' > FeatureFilterExprParser<Iterator>::r_featureset > ')' ) 
	  [ qi::_val = phx::construct<FilterPtr>(new_<faceAdjacentToFaces>(*qi::_1)) ]
      ;

      FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
	( lit("cylRadius") ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<cylRadius>()) ]
	|
	( lit("area") ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<faceArea>()) ]
      ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
        ( lit("normal") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::faceNormalVector>()) ]
        |
        ( lit("CoG") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::faceCoG>()) ]
      ;
    
  }
};


template <typename Iterator/*, typename Skipper = skip_grammar<Iterator>*/ >
struct SolidFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  SolidFeatureFilterExprParser(const FeatureSetParserArgList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
//     FeatureFilterExprParser<Iterator>::r_filter_functions = 
// 	( lit("isPlane") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Plane)) ]
// 	|
// 	( lit("isCylinder") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cylinder)) ]
// 	|
// 	( lit("isCone") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cone)) ]
// 	|
// 	( lit("isSphere") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Sphere)) ]
// 	|
// 	( lit("isTorus") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Torus)) ]
// 	|
// 	( lit("isBezierSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BezierSurface)) ]
// 	|
// 	( lit("isBSplineSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BSplineSurface)) ]
// 	|
// 	( lit("isSurfaceOfRevolution") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfRevolution)) ]
// 	|
// 	( lit("isSurfaceOfExtrusion") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfExtrusion)) ]
// 	|
// 	( lit("isOffsetSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OffsetSurface)) ]
// 	|
// 	( lit("isOtherSurface") ) [ qi::_val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OtherSurface)) ]
// 	|
// 	( lit("isPartOfSolid") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<isPartOfSolidFace>(*qi::_1)) ]
// 	|
// 	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<coincidentFace>(*qi::_1)) ]
// 	|
// 	( lit("adjacentToEdges") > '(' > FeatureFilterExprParser<Iterator>::r_featureset > ')' ) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<faceAdjacentToEdges>(*qi::_1)) ]
// 	|
// 	( lit("adjacentToFaces") > '(' > FeatureFilterExprParser<Iterator>::r_featureset > ')' ) 
// 	  [ qi::_val = phx::construct<FilterPtr>(new_<faceAdjacentToFaces>(*qi::_1)) ]
//       ;

//       FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
// 	( lit("cylRadius") ) [ qi::_val = phx::construct<scalarQuantityComputer::Ptr>(new_<cylRadius>()) ]
//       ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
        ( lit("CoG") ) [ qi::_val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::solidCoG>()) ]
      ;
    
  }
};

template<class Parser>
FilterPtr parseFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
try {
  Parser parser(refs);
//   skip_grammar<Iterator> skip;
  
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  
  std::string::iterator first=contents_raw.begin();
  std::string::iterator last=contents_raw.end();
  
  FilterPtr result;
  bool r = qi::phrase_parse
  (
      first, 
      last,
      parser,
      qi::space,
      result
  );
  
//   ModelStepsWriter writer;
//   parser.model_.modelstepSymbols.for_each(writer);

  if (first != last) // fail if we did not get a full match
  {
    int n=first-contents_raw.begin();
    throw insight::Exception( str(format("parsing of filtering expression '%s' failed after %d chars!")%contents_raw%n) );
    return FilterPtr();
  }
  
//   model = parser.model_;
  
  return result;
}
catch (insight::Exception e)
{
  std::cerr<<"Exception occurred: "<<e<<std::endl;
  throw e;
}

}

FilterPtr parseVertexFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<VertexFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseEdgeFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<EdgeFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseFaceFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<FaceFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseSolidFilterExpr(std::istream& in, const FeatureSetParserArgList& refs)
{
  return parseFilterExpr<SolidFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

}

}
