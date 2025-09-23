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
 */

#include "GeomAPI_IntCS.hxx"
#include "booleanintersection.h"
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/tools.h"
#include <boost/spirit/include/qi.hpp>
#include "base/translations.h"
#include "cadexception.h"

#include "datum.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


    
    
defineType(BooleanIntersection);
//addToFactoryTable(Feature, BooleanIntersection);
addToStaticFunctionTable(Feature, BooleanIntersection, insertrule);
addToStaticFunctionTable(Feature, BooleanIntersection, ruleDocumentation);

size_t BooleanIntersection::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  if (auto *f = boost::get<FeaturePtr>(&m2_)) h+=**f;
  else if (auto *p = boost::get<DatumPtr>(&m2_)) h+=**p;
  else throw insight::UnhandledSelection();
  return h.getHash()+DerivedFeature::calcHash();
}




BooleanIntersection::BooleanIntersection(const BooleanIntersection&o, TreeCloneMap& tcm)
    : DerivedFeature(o, tcm)
{
    if (auto *fp=boost::get<FeaturePtr>(&o.m2_))
    {
        m2_=tcm.clone(*fp);
    }
    else if (auto *dp=boost::get<DatumPtr>(&o.m2_))
    {
        m2_=tcm.clone(*dp);
    }
}

BooleanIntersection::BooleanIntersection(ConstFeaturePtr m1, FeaturePtr m2)
    : DerivedFeature(m1),
      m2_(m2)
{ 
    setFeatureSymbolName( "("+m1->featureSymbolName()+" & "+m2->featureSymbolName()+")" );
}




BooleanIntersection::BooleanIntersection(ConstFeaturePtr m1, DatumPtr m2pl)
    : DerivedFeature(m1),
      m2_(m2pl)
{
    setFeatureSymbolName( "("+m1->featureSymbolName()+" & datum)" );
}





void BooleanIntersection::build()
{
    ExecTimer t("BooleanIntersection::build() ["+featureSymbolName()+"]");
    
    if (!cache.contains(hash()))
    {
        if (auto *f = boost::get<FeaturePtr>(&m2_))
      {
              BRepAlgoAPI_Common intersector(*baseFeature(), **f);
              intersector.Build();
              if (!intersector.IsDone())
              {
                  throw CADException
                  (
                      shared_from_this(),
                      _("Could not perform intersection operation.")
                  );
              }
              setShape(intersector.Shape());
              cache.insert(shared_from_this());
          baseFeature()->unsetLeaf();
          (*f)->unsetLeaf();
      }
      else
      {
          if (auto *pl = boost::get<DatumPtr>(&m2_))
          {
              if (!(*pl)->providesPlanarReference())
                  throw CADException(shared_from_this(),
                                     _("intersection: given reference does not provide planar reference!"));

              if (baseFeature()->isSingleWire() || baseFeature()->isSingleEdge())
              {
                  TopoDS_Compound res;
                  BRep_Builder builder;
                  builder.MakeCompound( res );

                  Handle_Geom_Surface pln(new Geom_Plane( (*pl)->plane() ));
                  for (TopExp_Explorer ex(*baseFeature(), TopAbs_EDGE); ex.More(); ex.Next())
                  {
                      TopoDS_Edge e=TopoDS::Edge(ex.Current());
                      GeomAPI_IntCS	intersection;
                      double x0, x1;
                      intersection.Perform(BRep_Tool::Curve(e, x0, x1), pln);

                      // For debugging only
                      if (!intersection.IsDone() )
                          throw CADException(shared_from_this(),
                                             _("intersection: edge intersection not successful!"));

                      // Get intersection curve
                      for (int j=1; j<=intersection.NbPoints(); j++)
                      {
                          builder.Add(res, BRepBuilderAPI_MakeVertex(intersection.Point(j)));;
                      }
                  }

                  setShape(res);
              }
              else
              {
                  BRepAlgoAPI_Common intersector(
                              *baseFeature(),
                              BRepBuilderAPI_MakeFace((*pl)->plane()).Face() );
                  intersector.Build();
                  if (!intersector.IsDone())
                  {
                      throw CADException
                      (
                          shared_from_this(),
                          _("could not perform shape/plane intersection operation.")
                      );
                  }
                  TopoDS_Shape isecsh = intersector.Shape();

                  setShape(isecsh);
              }
              baseFeature()->unsetLeaf();
          }
          else
              throw CADException(shared_from_this(),
                                 _("intersection: tool object undefined!") );
      }
    }
    else
    {
        this->operator=(*cache.markAsUsed<BooleanIntersection>(hash()));
    }
}


void BooleanIntersection::operator=(const BooleanIntersection& o)
{
  m2_=o.m2_;
  DerivedFeature::operator=(o);
}


std::shared_ptr<BooleanIntersection> operator&(FeaturePtr m1, FeaturePtr m2)
{
    return BooleanIntersection::create(m1, m2);
}




/*! \page BooleanIntersection BooleanIntersection
  * Return the intersection between feat1 and feat2.
  *
  * Syntax:
  * ~~~~
  * ( <feature expression: feat1> & <feature expression: feat2> ) : feature
  * ~~~~
  */
void BooleanIntersection::insertrule(parser::ISCADParser& ruleset)
{
//   ruleset.modelstepFunctionRules.add
//   (
//     "",
//     typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(
//
//
//
//     ))
//   );
}




FeatureCmdInfoList BooleanIntersection::ruleDocumentation()
{
    return FeatureCmdInfoList();
}




}
}
