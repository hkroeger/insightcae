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
#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>

#include "featurefilter.h"
#include "solidmodel.h"

#include "gp_Cylinder.hxx"

#define BOOST_SPIRIT_USE_PHOENIX_V3

#include "boost/filesystem.hpp"
#include <boost/fusion/include/std_pair.hpp>
#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include <boost/fusion/adapted/boost_tuple.hpp>
#include "boost/variant.hpp"
#include "boost/ptr_container/ptr_map.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
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

bool AND::checkMatch(FeatureID feature) const
{
  return f1_->checkMatch(feature) && f2_->checkMatch(feature);
}

FilterPtr AND::clone() const
{
  return FilterPtr(new AND(*f1_, *f2_));
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

template<> coincident<Edge>::coincident(const SolidModel& m)
: m_(m),
  f_(m.allEdges())
{
}

template<>
bool coincident<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_)
  {
    TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
    TopoDS_Edge e2=TopoDS::Edge(m_.edge(f));
    match |= isPartOf(e2, e1);
  }
  
  return match;
}

template<> coincident<Face>::coincident(const SolidModel& m)
: m_(m),
  f_(m.allFaces())
{
}

template<>
bool coincident<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_)
  {
    TopoDS_Face e1=TopoDS::Face(model_->face(feature));
    TopoDS_Face e2=TopoDS::Face(m_.face(f));
    match |= isPartOf(e2, e1);
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
  
  return (1.0 - fabs(arma::dot( arma::normalise(v), arma::normalise(dir_) ))) < 1e-10;
}


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

// template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
// struct FeatureFilterExprParser
//   : qi::grammar<Iterator, Skipper>
// {
// 
//     FilterPtr filter_;
// 
//     qi::rule<Iterator, scalar(), Skipper> r_scalar_primary, r_scalar_term, r_scalarExpression;
//     qi::rule<Iterator, vector(), Skipper> r_vector_primary, r_vector_term, r_vectorExpression;
// 
//     qi::rule<Iterator, Skipper> r_filter;
// 
// 
//     FeatureFilterExprParser()
//         : FeatureFilterExprParser::base_type(r_filter),
//           filter_()
//     {
// 
//         using namespace qi;
//         using namespace phx;
//         using namespace insight::cad;
// 
//         r_filter =  *( r_assignment | r_modelstep | r_loadmodel )
//                     >> -( lit("@post")  >> *r_postproc);
// 
// 
//         on_error<fail>(r_model,
//                        phx::ref(std::cout)
//                        << "Error! Expecting "
//                        << qi::_4
//                        << " here: '"
//                        << phx::construct<std::string>(qi::_3, qi::_2)
//                        << "'\n"
//                       );
//     }  
//     
// };
FilterPtr parseFilterExpr(const std::istream& stream)
{
}

}

}