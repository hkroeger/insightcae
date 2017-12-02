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
#include <boost/spirit/include/qi.hpp>

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
addToFactoryTable(Feature, BooleanIntersection);




BooleanIntersection::BooleanIntersection()
    : DerivedFeature()
{}




BooleanIntersection::BooleanIntersection(FeaturePtr m1, FeaturePtr m2)
    : DerivedFeature(m1),
      m1_(m1),
      m2_(m2)
{
    ParameterListHash h(this);
    h+=this->type();
    h+=*m1_;
    h+=*m2_;   
    setFeatureSymbolName( "("+m1->featureSymbolName()+" & "+m2->featureSymbolName()+")" );
}




BooleanIntersection::BooleanIntersection(FeaturePtr m1, DatumPtr m2pl)
    : DerivedFeature(m1),
      m1_(m1),
      m2pl_(m2pl)
{
    setFeatureSymbolName( "("+m1->featureSymbolName()+" & datum)" );
}




FeaturePtr BooleanIntersection::create(FeaturePtr m1, FeaturePtr m2)
{
    return FeaturePtr(new BooleanIntersection(m1, m2));
}




FeaturePtr BooleanIntersection::create_plane(FeaturePtr m1, DatumPtr m2pl)
{
    return FeaturePtr(new BooleanIntersection(m1, m2pl));
}




void BooleanIntersection::build()
{
    if (m1_ && m2_)
    {        
        if (!cache.contains(hash()))
        {
            BRepAlgoAPI_Common intersector(*m1_, *m2_);
            intersector.Build();
            if (Standard_Integer err = intersector.ErrorStatus() != 0)
            {
                throw CADException
                (
                    *this,
                    boost::str(boost::format("could not perform intersection operation: error code %d.")
                    % err )
                );
            }
            setShape(intersector.Shape());
            cache.insert(shared_from_this());
        }
        else
        {
            this->operator=(*cache.markAsUsed<BooleanIntersection>(hash()));
        }
        m1_->unsetLeaf();
        m2_->unsetLeaf();
    }
    else
    {
        if (m2pl_)
        {
            if (!m2pl_->providesPlanarReference())
                throw CADException(*this, "intersection: given reference does not provide planar reference!");

            if (m1_->isSingleWire() || m1_->isSingleEdge())
            {
                TopoDS_Compound res;
                BRep_Builder builder;
                builder.MakeCompound( res );

                Handle_Geom_Surface pl(new Geom_Plane(m2pl_->plane()));
                for (TopExp_Explorer ex(*m1_, TopAbs_EDGE); ex.More(); ex.Next())
                {
                    TopoDS_Edge e=TopoDS::Edge(ex.Current());
                    GeomAPI_IntCS	intersection;
                    double x0, x1;
                    intersection.Perform(BRep_Tool::Curve(e, x0, x1), pl);

                    // For debugging only
                    if (!intersection.IsDone() )
                        throw CADException(*this, "intersection: edge intersection not successful!");

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
                BRepAlgoAPI_Section intersector
                (
                    *m1_,
                    m2pl_->plane()
                );
                intersector.Build();
                if (Standard_Integer err = intersector.ErrorStatus() != 0)
                {
                    throw CADException
                    (
                        *this,
                        boost::str(boost::format("could not perform shape/plane intersection operation: error code %d.")
                        % err )
                    );
                }                
                TopoDS_Shape isecsh = intersector.Shape();

                setShape(isecsh);
            }
            m1_->unsetLeaf();
        }
        else
            throw CADException(*this, "intersection: tool object undefined!");
    }
}


void BooleanIntersection::operator=(const BooleanIntersection& o)
{
  m1_=o.m1_;
  m2_=o.m2_;
  m2pl_=o.m2pl_;
  Feature::operator=(o);
}


FeaturePtr operator&(FeaturePtr m1, FeaturePtr m2)
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
void BooleanIntersection::insertrule(parser::ISCADParser& ruleset) const
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




FeatureCmdInfoList BooleanIntersection::ruleDocumentation() const
{
    return FeatureCmdInfoList();
}




}
}
