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

#include "arc.h"
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"
#include "base/translations.h"

#include "boost/phoenix/object/dynamic_cast.hpp"
#include "constrainedsketch.h"
#include "datum.h"

#include <boost/spirit/include/qi.hpp>
#include <memory>

#ifndef Q_MOC_RUN
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#endif
#include <boost/spirit/home/support/iterators/line_pos_iterator.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
  
    
    
defineType(Arc);


addToStaticFunctionTable(Feature, Arc, insertrule);
addToStaticFunctionTable(Feature, Arc, ruleDocumentation);


size_t Arc::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*p0_;
  h+=*p1_;
  h+=avType_;
  h+=*av_;
  return h.getHash();
}







bool Arc::isSingleOpenWire() const
{
  return true;
}





Handle_Geom_TrimmedCurve Arc::calcArc() const
{
    Handle_Geom_TrimmedCurve crv;

    auto n_r = [this](arma::mat n_times_r)
    {
        double alpha=arma::norm(n_times_r,2);
        arma::mat n=normalized(n_times_r);

        arma::mat p0=p0_->value();
        arma::mat p1=p1_->value();
        arma::mat S=p1-p0;

        double s=arma::norm(S,2);
        double r=s/2./sin(alpha/2.);

        double a=sqrt(r*r-0.25*s*s);
        arma::mat ctr= p0 + S*0.5 + normalized(arma::cross(n,S))*a *(alpha<M_PI?1.:-1.);

        std::cout<<p0.t()<<p1.t()<<S.t()<<r<<" "<<a<<"\n"<<ctr.t();

        return GC_MakeArcOfCircle(
            gp_Circ(gp_Ax2(to_Pnt(ctr), to_Dir(n), to_Dir(normalized(p0-ctr))), r),
            to_Pnt(p0_->value()),
            to_Pnt(p1_->value()),
            true
        ).Value();
    };

    if (auto skmp=std::dynamic_pointer_cast<ArcCenterPoint>(av_))
    {
       crv=n_r(skmp->normalTimesAngle());
    }
    else
    {
        switch (avType_)
        {
            case NormalTimesRadius:
                crv=n_r(av_->value());
                break;
            case IntermediatePoint:
                crv=GC_MakeArcOfCircle(
                        to_Pnt(p0_->value()),
                        to_Pnt(av_->value()),
                        to_Pnt(p1_->value())
                    );
                break;
            case P0Tangent:
                crv=GC_MakeArcOfCircle(
                    to_Pnt(p0_->value()),
                    to_Vec(av_->value()),
                    to_Pnt(p1_->value())
                    );
                break;
            case P1Tangent:
                crv=GC_MakeArcOfCircle(
                    to_Pnt(p1_->value()),
                    to_Vec(av_->value()),
                    to_Pnt(p0_->value())
                    );
                break;
        }
    }
    return crv;
}




void Arc::build()
{
  auto crv = calcArc();
  
  setShape(BRepBuilderAPI_MakeEdge(crv));

  auto c = Handle_Geom_Circle::DownCast(crv->BasisCurve())->Circ();

  arma::mat ctr=vec3(c.Location());

  refpoints_["center"]=ctr;
  refvalues_["D"]=c.Radius()*2.;
  refvectors_["normal"]=vec3(c.Axis().Direction());
}


Arc::Arc(const Arc&o, TreeCloneMap& tcm)
    : CL(p0_), CL(p1_), avType_(o.avType_), CL(av_)
{
    ConstrainedSketchEntity::operator=(o);
}

Arc::Arc(
    VectorPtr p0,
    ThirdVectorType avType, VectorPtr av,
    VectorPtr p1,
    const std::string& layerName )
: ConstrainedSketchEntity(layerName),
    p0_(p0), avType_(avType), av_(av), p1_(p1)
{}



std::shared_ptr<Arc> Arc::create(
    VectorPtr p0,
    ThirdVectorType avType, VectorPtr av,
    VectorPtr p1,
    const std::string &layerName)
{
    auto f=std::shared_ptr<Arc>(new Arc(p0, avType, av, p1, layerName));
    if (auto skmp = std::dynamic_pointer_cast<ArcCenterPoint>(av))
    {
        skmp->linktoArc( f );
    }
    return f;
}




void Arc::insertrule(parser::ISCADParser& ruleset)
{
    using boost::spirit::repository::qi::iter_pos;

    ruleset.modelstepFunctionRules.add
        (
            "Arc3P",
            typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                ( '(' > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ')' )
                    [ qi::_val = phx::bind(
                         &Arc::create,
                         qi::_1, IntermediatePoint, qi::_2, qi::_3, std::string()) ]
                ))
            );

    ruleset.modelstepFunctionRules.add
        (
            "Arc",
            std::make_shared<parser::ISCADParser::ModelstepRule>(

                ( '(' > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ')' )
                    [ qi::_val = phx::bind(
                         &Arc::create,
                         qi::_1, P0Tangent, qi::_2, qi::_3, std::string()) ]

                )
            );
}




FeatureCmdInfoList Arc::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Arc3P",
            "( <vector:p0>, <vector:pm>, <vector:p1> )",
            _("Creates an arc between point p0 and p1 through intermediate point pm.")
        ),
        FeatureCmdInfo
        (
            "Arc",
            "( <vector:p0>, <vector:et0>, <vector:p1> )",
            _("Creates an arc between point p0 and p1. At point p0, the arc is tangent to vector et0.")
        )
    };
}





void Arc::scaleSketch(double scaleFactor)
{}




void Arc::generateScriptCommand(
    ConstrainedSketchScriptBuffer& script,
    const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const
{
    insight::assertion(avType_==NormalTimesRadius, "unexpected third point type");

    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "("
            + toString(myLabel) +", "
            + pointSpec(p0_, script, entityLabels) + ", "
            + pointSpec(av_, script, entityLabels) + ", "
            + pointSpec(p1_, script, entityLabels) + ", "
            + "layer " + layerName()
            + parameterString()
            + ")"
        );
}




void Arc::addParserRule(
    ConstrainedSketchGrammar& ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    namespace qi=boost::spirit::qi;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > ruleset.r_point > ','
             > ruleset.r_point > ','
             > ruleset.r_point
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters > ')'
             )
                [   qi::_a = phx::bind(
                     &Arc::create,
                     qi::_2, NormalTimesRadius, qi::_3, qi::_4, qi::_5),
                 phx::bind( &ArcCenterPoint::linktoArc,
                       phx::dynamic_cast_<ArcCenterPoint*>(&*qi::_3),
                       phx::bind(&std::dynamic_pointer_cast<Arc,ConstrainedSketchEntity>, qi::_a) ),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters,
                           &pd, phx::ref(*qi::_a) ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet,
                           qi::_a, qi::_6, boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(
                     qi::_1, qi::_a) ]
            );
}




std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
Arc::dependencies() const
{
    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > ret;

    if (auto sp1=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p0_))
        ret.insert(sp1);
    if (auto spm=std::dynamic_pointer_cast<ConstrainedSketchEntity>(av_))
        ret.insert(spm);
    if (auto sp2=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_))
        ret.insert(sp2);

    return ret;
}




void Arc::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity>& entity,
    const std::shared_ptr<ConstrainedSketchEntity>& newEntity)
{
    if (auto p = std::dynamic_pointer_cast<Vector>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p0_) == entity )
        {
            p0_ = p;
            invalidate();
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(av_) == entity )
        {
            av_ = p;
            invalidate();
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_) == entity )
        {
            p1_ = p;
            invalidate();
        }
    }
    invalidate();
}






bool Arc::isInside( SelectionRect r) const
{
    return
        r.isInside(p0_->value())
        && r.isInside(p1_->value());
}




bool Arc::pointIsOnLine(const arma::mat& p) const
{
    auto crv = calcArc();

    GeomAPI_ProjectPointOnCurve proj(to_Pnt(p), crv);
    int np = proj.NbPoints();
    insight::assertion(np>0, "nearest point determination failed");
    for (int i=1; i<=np; i++)
        if (proj.Distance(i)<SMALL) return true;
    return false;
}




arma::mat Arc::projectOntoLine(const arma::mat& p) const
{
    auto crv = calcArc();

    GeomAPI_ProjectPointOnCurve proj(to_Pnt(p), crv);
    int np = proj.NbPoints();
    insight::assertion(np>0, "nearest point determination failed");
    std::map<double, gp_Pnt> pts;
    for (int i=1; i<=np; i++)
        pts[proj.Distance(i)]=proj.Point(i);
    return insight::Vector(pts.begin()->second);
}




void Arc::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const Arc&>(other));
}




void Arc::operator=(const Arc& other)
{
    p0_=other.p0_;
    avType_=other.avType_;
    av_=other.av_;
    p1_=other.p1_;
    SingleEdgeFeature::operator=(other);
    ConstrainedSketchEntity::operator=(other);
}




ConstrainedSketchEntityPtr Arc::clone() const
{
    auto cl=Arc::create(
        p0_, avType_, av_, p1_,
        layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef().assignFrom( parameters() );
    return cl;
}




std::vector<vtkSmartPointer<vtkProp> > Arc::createActor() const
{
    return Feature::createVTKActors();
}







defineType(ArcCenterPoint);
addToStaticFunctionTable(ConstrainedSketchEntity, ArcCenterPoint, addParserRule);


ArcCenterPoint::ArcCenterPoint(
    DatumPtr plane, double angle,
    const std::string& layerName)
  : SketchPoint(
          plane,
          0, 0,
          layerName),
    angle_(angle)
{}

// ArcCenterPoint::ArcCenterPoint(DatumPtr plane, double x, double y, const std::string& layerName)
//     : SketchPoint(plane, x, y, layerName)
// {}


void ArcCenterPoint::linktoArc(std::weak_ptr<Arc> arc)
{
    arc_=arc;
    coords2D(); // trigger recalc
}


arma::mat ArcCenterPoint::calcXYFromArc() const
{
    insight::assertion(
        !arc_.expired(), "no link to arc is set");

    auto a=arc_.lock();
    auto c=a->getDatumPoint("center");

    return ConstrainedSketch::p3Dto2D(
        plane()->plane(), c );
}


void ArcCenterPoint::setCoords2D(double x, double y)
{
    // cannot be changed
}


arma::mat ArcCenterPoint::coords2D() const
{
    arma::mat p2 = calcXYFromArc();
    const_cast<ArcCenterPoint*>(this)
        -> SketchPoint::setCoords2D(p2(0), p2(1));

    return SketchPoint::coords2D();
}


arma::mat ArcCenterPoint::normalTimesAngle() const
{
    return vec3(plane()->plane().Direction().XYZ())*angle_;
}

int ArcCenterPoint::nDoF() const
{
    return 1;
}


double ArcCenterPoint::getDoFValue(unsigned int iDoF) const
{
    switch (iDoF)
    {
    case 0: return angle_; break;
    default:
        throw insight::Exception(
            "invalid DoF index: %d", iDoF );
    }
    return NAN;
}


void ArcCenterPoint::setDoFValue(unsigned int iDoF, double value)
{
    switch (iDoF)
    {
    case 0: angle_=value; break;
    default:
        throw insight::Exception(
            "invalid DoF index: %d", iDoF );

    }
}

void ArcCenterPoint::scaleSketch(double scaleFactor)
{
    angle_ *= scaleFactor;
}



void ArcCenterPoint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    auto v = coords2D();
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + toString(myLabel) + ", "
            + str(boost::format("%g, %g")%v(0)%v(1))
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}

void ArcCenterPoint::addParserRule(
    ConstrainedSketchGrammar& ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::double_ > ',' > qi::double_
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')' )
                [ qi::_a = phx::bind(
                     &SketchPoint::create<DatumPtr, double, double, const std::string&>,
                     ruleset.sketch->plane(), qi::_2, qi::_3, qi::_4),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, phx::ref(*qi::_a)),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5,
                           boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}



void ArcCenterPoint::operator=(const ArcCenterPoint& other)
{
    angle_=other.angle_;
    SketchPoint::operator=(other);
}

void ArcCenterPoint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const ArcCenterPoint&>(other));
}

ConstrainedSketchEntityPtr ArcCenterPoint::clone() const
{
    auto cl=ArcCenterPoint::create(
        plane(),
        angle_,
        layerName() );
    cl->linktoArc(arc_);
    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef().assignFrom( parameters() );
    return cl;
}



}
}
