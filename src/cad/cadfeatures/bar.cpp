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

#include "bar.h"
#include "base/exception.h"
#include "feature.h"
#include "quad.h"
#include "line.h"
#include "transform.h"

#include "base/boost_include.h"
#include "base/translations.h"
#include <boost/spirit/include/qi.hpp>

#include "cadfeatures/singleedgefeature.h"
#include "cadfeatures/importsolidmodel.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

//using namespace std;
//using namespace boost;

namespace boost {
namespace fusion {
 typedef boost::fusion::vector3<insight::cad::ScalarPtr, insight::cad::ScalarPtr, insight::cad::ScalarPtr> Arg;
 void swap(Arg&a1, Arg&a2)
 {
  std::swap(a1, a2);
 }
}
}

namespace insight {
namespace cad {

  
defineType(Bar);
//addToFactoryTable(Feature, Bar);


size_t Bar::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  if (const auto* p0p1 =
          boost::get<std::pair<VectorPtr,VectorPtr>>(&endPts_))
  {
      h+=p0p1->first->value();
      h+=p0p1->second->value();
  }
  else if (const auto* feat =
           boost::get<FeaturePtr>(&endPts_))
  {
      h+=**feat;
  }
  h+=*xsec_;
  if (thickness_) h+=thickness_->value();
  h+=vert_->value();
  h+=ext0_->value();
  h+=ext1_->value();
  h+=miterangle0_vert_->value();
  h+=miterangle1_vert_->value();
  h+=miterangle0_hor_->value();
  h+=miterangle1_hor_->value();
  if (attractPt_) h+=attractPt_->value();
  return h.getHash();
}


void Bar::build()
{

    if (!cache.contains(hash()))
    {

        arma::mat p0, p1;

        if (const auto* p0p1 =
                boost::get<std::pair<VectorPtr,VectorPtr>>(&endPts_))
        {
            p0 = p0p1->first->value();
            p1 = p0p1->second->value();
        }
        else if (const auto* feat =
                 boost::get<FeaturePtr>(&endPts_))
        {
            if (auto sef = std::dynamic_pointer_cast<SingleEdgeFeature>(*feat))
            {
                p0 = sef->start()->value();
                p1 = sef->end()->value();
            }
            else
            {
                p0 = (*feat)->getDatumPoint("p0");
                p1 = (*feat)->getDatumPoint("p1");
            }
        }
        refpoints_["p0"]=p0;
        refpoints_["p1"]=p1;

        if (norm(vert_->value(),2)<1e-10)
            throw insight::Exception(_("Bar: length of vertical direction is zero!"));
        arma::mat v=vert_->value() / norm(vert_->value(),2);
        /*
              if (!xsec_->isSingleFace() || xsec_->isSingleWire() || xsec_->isSingleEdge())
        	throw insight::Exception("xsec feature has to provide a face or wire!");
          */
        arma::mat baraxis=p1-p0;
        double lba=norm(baraxis,2);
        if (lba<1e-10)
            throw insight::Exception(_("Bar: invalid definition of bar end points!"));
        baraxis/=lba;

        p0 += -baraxis*(*ext0_);
        p1 +=  baraxis*(*ext1_);
        double L=norm(p1-p0, 2);
        insight::assertion(L>1e-10, _("the bar length must not be zero!"));

        refpoints_["start"]=p0;
        refpoints_["end"]=p1;

        refvectors_["ex"]=(p1 - p0)/L;

        refvalues_["L"]=L;

        TopoDS_Wire spine=BRepBuilderAPI_MakeWire
                          (
                              BRepBuilderAPI_MakeEdge
                              (
                                  GC_MakeSegment(to_Pnt(p0), to_Pnt(p1)).Value()
                              )
                          );
        //   TopoDS_Vertex pfirst, plast;
        //   TopExp::Vertices( spine, pfirst, plast );


        arma::mat ex=-arma::cross(baraxis, vert_->value());

        double lex=norm(ex, 2);
        if (lex<1e-10)
            throw insight::Exception(_("Bar: invalid definition of vertical direction!"));
        ex/=lex;

        arma::mat ey=arma::cross(baraxis, ex);

        setLocalCoordinateSystem(p0, baraxis, arma::cross(baraxis,ey));

        gp_Trsf tr;
        tr.SetTransformation
        (
            // from
            gp_Ax3
            (
                gp_Pnt(0,0,0),
                gp_Dir(0,0,1),
                gp_Dir(1,0,0)
            ),
            //to
            gp_Ax3
            (
                to_Pnt(p0),
                to_Vec(baraxis),
                to_Vec(ex)
            )
        );
        
        auto xsect=Transform::create(xsec_, tr.Inverted());

        bool isThinFeature = xsect->topologicalProperties().onlyEdges();

        if (thickness_ && !isThinFeature)
            throw insight::CADException(
                {{"cross section", xsect}},
                "Thickness was specified by the section does not comprise of edges only!"
                " Please check modelling intention."
                );

        if (attractPt_)
        {
            auto vidx=xsect->query_vertices("maximal(dist(loc,%m0))", {attractPt_});
            insight::assertion(
                vidx.size()>0,
                "no reference vertex found!");

            auto pzero = vec3(BRep_Tool::Pnt(xsect->vertex(*vidx.begin())));
            xsect=Transform::create(xsect, cad::matconst(-(pzero-p0)));
        }

        providedSubshapes_["xsec"]=xsect;
        providedSubshapes_["spine"]=Line::create(matconst(p0), matconst(p1));
        TopoDS_Shape xsecs = *xsect;

        //   BRepOffsetAPI_MakePipeShell p(spinew);
        //   Handle_Law_Constant law(new Law_Constant());
        //   law->Set(1.0, -1e10, 1e10);
        //   p.SetLaw(static_cast<TopoDS_Shape>(xsec), law, pfirst);
        //   p.SetMode(true);
        //   p.MakeSolid();

        BRepOffsetAPI_MakePipe p(spine, xsecs);

        p.Build();
        TopoDS_Shape result=p.Shape();

        // cut away at end 0
        if ( (fabs(*miterangle0_vert_)>1e-10) || (fabs(*miterangle0_hor_)>1e-10) )
        {
            arma::mat cex=rotMatrix(*miterangle0_vert_, ey)*ex;
            arma::mat cey=rotMatrix(*miterangle0_hor_, ex)*ey;
            FeaturePtr q=Quad::create
                         (
                             matconst(p0 -0.5*L*(cex+cey)),
                             matconst(L*cex),
                             matconst(L*cey)
                         );
            TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q->shape()), to_Vec(-L*baraxis) );
            result=BRepAlgoAPI_Cut(result, airspace);
        }

        // cut away at end 1
        if ( (fabs(*miterangle1_vert_)>1e-10) || (fabs(*miterangle1_hor_)>1e-10) )
        {
            arma::mat cex=rotMatrix(*miterangle1_vert_, ey)*ex;
            arma::mat cey=rotMatrix(*miterangle1_hor_, ex)*ey;
            FeaturePtr q=Quad::create
                         (
                             matconst(p1 -0.5*L*(cex+cey)),
                             matconst(L*cex),
                             matconst(L*cey)
                         );
            TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q->shape()), to_Vec(L*baraxis) );
            result=BRepAlgoAPI_Cut(result, airspace);
        }


        if (thickness_)
        {

            auto sf=Import::create(result);
            providedSubshapes_["shell"]=sf;

            auto longedgs=cad::makeEdgeFeatureSet(
                sf, "angleMag(end-start, %m0)<0.001", {cad::matconst(p1 - p0)});
            providedFeatureSets_["e_shell_longi"]=longedgs;
            arma::mat midp=0.5*(p0+p1);
            providedFeatureSets_["e_shell_start"]=
                cad::makeEdgeFeatureSet(
                    sf, "(!in(%0)) && (((CoG-%m1)&%m2)<0)",
                {longedgs, cad::matconst(midp), cad::matconst(normalized(p1 - p0))} );
            providedFeatureSets_["e_shell_end"]=
                cad::makeEdgeFeatureSet(
                    sf, "(!in(%0)) && (((CoG-%m1)&%m2)>0)",
                    {longedgs, cad::matconst(midp), cad::matconst(normalized(p1 - p0))} );



            BRepOffset_MakeOffset maker;
            maker.Initialize
                (
                result, thickness_->value(),
                Precision::Confusion(),
                BRepOffset_Skin, Standard_True, Standard_True, GeomAbs_Arc, Standard_True
                );
            for (TopExp_Explorer ex(result, TopAbs_FACE); ex.More(); ex.Next())
            {
                maker.SetOffsetOnFace(TopoDS::Face(ex.Current()), thickness_->value());
            }


            maker.MakeThickSolid();

            result=maker.Shape();

            ShapeFix_Solid FixShape;
            FixShape.Init(TopoDS::Solid(result));
            FixShape.Perform();
            result=FixShape.Shape();
        }

        setShape(result);

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<Bar>(hash()));
    }
}






Bar::Bar
(
  EndPoints endPts,
  FeaturePtr xsec, VectorPtr vert, 
  ScalarPtr ext0, ScalarPtr ext1,
  ScalarPtr miterangle0_vert, ScalarPtr miterangle1_vert,
  ScalarPtr miterangle0_hor, ScalarPtr miterangle1_hor,
    VectorPtr attractPt
)
: endPts_(endPts),
  xsec_(xsec),
  vert_(vert),
  ext0_(ext0),
  ext1_(ext1),
  miterangle0_vert_(miterangle0_vert),
  miterangle1_vert_(miterangle1_vert),
  miterangle0_hor_(miterangle0_hor),
  miterangle1_hor_(miterangle1_hor),
    attractPt_(attractPt)
{
}




Bar::Bar
(
  EndPoints endPts,
  FeaturePtr xsec, VectorPtr vert, 
  const boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>& ext_miterv_miterh0, 
  const boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>& ext_miterv_miterh1,
    VectorPtr attractPt
)
: endPts_(endPts),
  xsec_(xsec),
  vert_(vert),
  ext0_(boost::fusion::at_c<0>(ext_miterv_miterh0)),
  ext1_(boost::fusion::at_c<0>(ext_miterv_miterh1)),
  miterangle0_vert_(boost::fusion::at_c<1>(ext_miterv_miterh0)),
  miterangle1_vert_(boost::fusion::at_c<1>(ext_miterv_miterh1)),
  miterangle0_hor_(boost::fusion::at_c<2>(ext_miterv_miterh0)),
  miterangle1_hor_(boost::fusion::at_c<2>(ext_miterv_miterh1)),
    attractPt_(attractPt)
{
}



Bar::Bar
(
    EndPoints endPts,
    FeaturePtr xsec, ScalarPtr thickness,
    VectorPtr vert,
    const boost_EndPointMod& ext_miterv_miterh0,
    const boost_EndPointMod& ext_miterv_miterh1,
    VectorPtr attractPt
)
: endPts_(endPts),
    xsec_(xsec),
    thickness_(thickness),
    vert_(vert),
    ext0_(boost::fusion::at_c<0>(ext_miterv_miterh0)),
    ext1_(boost::fusion::at_c<0>(ext_miterv_miterh1)),
    miterangle0_vert_(boost::fusion::at_c<1>(ext_miterv_miterh0)),
    miterangle1_vert_(boost::fusion::at_c<1>(ext_miterv_miterh1)),
    miterangle0_hor_(boost::fusion::at_c<2>(ext_miterv_miterh0)),
    miterangle1_hor_(boost::fusion::at_c<2>(ext_miterv_miterh1)),
    attractPt_(attractPt)
{}






std::shared_ptr<Bar> Bar::create_derived
(
    FeaturePtr skel,
    FeaturePtr xsec, VectorPtr vert,
    const EndPointMod& epm0,
    const EndPointMod& epm1,
    VectorPtr attractPt
)
{
    return std::shared_ptr<Bar>( new Bar
               (
                   skel,
                   xsec, vert,

                   boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>
                    {epm0.ext, epm0.miterAngleVert, epm0.miterAngleHorz},

                   boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>
                    {epm1.ext, epm1.miterAngleVert, epm1.miterAngleHorz},

                    attractPt
               )
            );
}



void Bar::operator=(const Bar& o)
{
  endPts_=o.endPts_;
  thickness_=o.thickness_;
  xsec_=o.xsec_;
  vert_=o.vert_;
  ext0_=o.ext0_;
  ext1_=o.ext1_;
  miterangle0_vert_=o.miterangle0_vert_;
  miterangle1_vert_=o.miterangle1_vert_;
  miterangle0_hor_=o.miterangle0_hor_;
  miterangle1_hor_=o.miterangle1_hor_;
  attractPt_=o.attractPt_;
  Feature::operator=(o);
}




/**
 * \page iscad_bar Bar
 *
 * The "Bar" command creates a straight extruded volume from a planar section. 
 * The location of the bar is specified by its start point "p0" and end point "p1".
 * Also needed is a face feature defining the cross section.
 * For orienting the cross section an upward direction needs to be given.
 * 
 * The end points can be shifted along the bar axis by offsets "ext0" and "ext1".
 * Furthermore, the bar ends can be mitered around the vertical axis ("vmiter0" and "vmiter1") and the horizontal axis ("hmiter0" and "hmiter1").
 * 
 * \section syntax Syntax
 * 
 * <b> Bar(
 *  \ref iscad_vector_expression "<vector:p0>" [\ref iscad_scalar_expression "<scalar:ext0>"] [\ref iscad_scalar_expression "<scalar:vmiter0>"] [\ref iscad_scalar_expression "<scalar:hmiter0>"],
 *  \ref iscad_vector_expression "<vector:p1>" [\ref iscad_scalar_expression "<scalar:ext1>"] [\ref iscad_scalar_expression "<scalar:vmiter1>"] [\ref iscad_scalar_expression "<scalar:hmiter1>"],
 *  \ref iscad_feature_expression "<feature:xsec>",
 *  \ref iscad_vector_expression "<vector:up>"
 * ) </b>
 * 
 * \section return Return Value
 * 
 * \ref iscad_feature_expression "Feature"
 * 
 * \section provides Provided Properties
 * 
 * Points:
 * * "start": uncorrected start point
 * * "end": uncorrected end point
 * * "p0": offset start point
 * * "p1": offset end point
 * 
 * Scalars:
 * * "L": length of the bar axis
 */

addToStaticFunctionTable(Feature, Bar, insertrule);
addToStaticFunctionTable(Feature, Bar, ruleDocumentation);


void Bar::insertrule(parser::ISCADParser& ruleset)
{
    typedef
    qi::rule<
            std::string::iterator,
            EndPointMod(),
            parser::ISCADParser::skipper_type
            >

            EndPtRule;

    auto *r_endpt = new EndPtRule(
            *(
                ( qi::lit("ext") > ruleset.r_scalarExpression
                   [ phx::bind(&EndPointMod::ext, qi::_val) = qi::_1 ])
                |
                ( qi::lit("vmiter") > ruleset.r_scalarExpression
                  [ phx::bind(&EndPointMod::miterAngleVert, qi::_val) = qi::_1 ] )
                |
                ( qi::lit("hmiter") > ruleset.r_scalarExpression
                  [ phx::bind(&EndPointMod::miterAngleHorz, qi::_val) = qi::_1 ] )
            )
    );
    ruleset.addAdditionalRule(r_endpt);


    ruleset.modelstepFunctionRules.add
    (
        "Bar",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                    ( '(' > (
                      (
                             ruleset.r_vectorExpression // 1
                          >> qi::hold[ (  (( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                          >> ((  qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                          >> ((  qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))  ) ]
                          >> ','
                          >> ruleset.r_vectorExpression // 3
                          >> qi::hold[ (  (( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                          >> ((  qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                          >> ((  qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))  ) ]
                          >> ','
                          >> ruleset.r_solidmodel_expression >> ',' // 5
                          >> ruleset.r_vectorExpression // 6
                        >> ( ( ',' > ruleset.r_vectorExpression) | qi::attr(VectorPtr()) ) >> // 7
                          ')' )
                        [ qi::_val = phx::bind(&Bar::create<
                                                  EndPoints,
                                                  FeaturePtr, VectorPtr,
                                                  const boost_EndPointMod&,
                                                  const boost_EndPointMod&,
                                                  VectorPtr>,
                                              phx::construct<Bar::EndPoints>(
                                                  phx::construct<std::pair<VectorPtr,VectorPtr> >(
                                                      qi::_1, qi::_3 ) ),
                                              qi::_5, qi::_6,
                                              qi::_2, qi::_4,
                                              qi::_7
                                     ) ]
                    |
                    (
                            ruleset.r_solidmodel_expression > ','// 1
                         > ( ( qi::lit("start") > *r_endpt > ',' ) | qi::attr(EndPointMod()) )
                         > ( ( qi::lit("end") > *r_endpt > ',' ) | qi::attr(EndPointMod()) )
                         > ruleset.r_solidmodel_expression > ',' // 4
                         > ruleset.r_vectorExpression // 5
                       > ( ( ',' > ruleset.r_vectorExpression) | qi::attr(VectorPtr()) ) //6
                         > ')' )
                     [ qi::_val = phx::bind(&Bar::create_derived,
                                                             qi::_1,
                                                             qi::_4, qi::_5,
                                                             qi::_2, qi::_3,
                                                             qi::_6
                                                         ) ] )
                ) ))
    );

    ruleset.modelstepFunctionRules.add
        (
            "SheetBar",
            typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                ( '(' >
                     (
                         ruleset.r_vectorExpression // 1
                         >> qi::hold[ (  (( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                      >> ((  qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                      >> ((  qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))  ) ]
                         >> ','
                         >> ruleset.r_vectorExpression // 3
                         >> qi::hold[ (  (( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                      >> ((  qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))
                                      >> ((  qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0)))  ) ]
                         >> ','
                         >> ruleset.r_solidmodel_expression >> ',' // 5
                         >> ruleset.r_scalarExpression >> ',' // 6
                         >> ruleset.r_vectorExpression // 7
                         >> ( ( ',' > ruleset.r_vectorExpression) | qi::attr(VectorPtr()) ) >> // 8
                         ')' )
                        [ qi::_val = phx::bind(&Bar::create<
                                                  EndPoints, FeaturePtr,
                                                  ScalarPtr, VectorPtr,
                                                  const boost_EndPointMod&,
                                                  const boost_EndPointMod&,
                                                  VectorPtr>,
                                              phx::construct<Bar::EndPoints>(
                                               phx::construct<std::pair<VectorPtr,VectorPtr> >(
                                                 qi::_1, qi::_3 ) ),
                                              qi::_5, qi::_6, qi::_7,
                                               qi::_2, qi::_4,
                                               qi::_8
                                               ) ]
                 ) ) )
        );
}

FeatureCmdInfoList Bar::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Bar",
         
            "( <vector:p0> [ext <scalar:ext0>] [vmiter <scalar:vmiter0>] [hmiter <scalar:hmiter0>],\n"
            "<vector:p1> [ext <scalar:ext1>] [vmiter <scalar:vmiter1>] [hmiter <scalar:hmiter1>],\n"
            "<feature:xsec>, <vector:up> )",
         
            _("This command creates a straight extruded volume from a planar section."
            "\n"
            "The location of the bar is specified by its start point p0 and end point p1."
            " Also needed is a face feature xsec defining the cross section."
            " For orienting the cross section an upward direction up has to be given."
            "\n"
            "The end points can optionally be shifted along the bar axis by offsets ext0 and ext1."
              " Furthermore, the bar ends can be mitered around the vertical axis (vmiter0, vmiter1) and the horizontal axis (hmiter0, hmiter1).")
        ),

        FeatureCmdInfo
        (
            "SheetBar",

            "( <vector:p0> [ext <scalar:ext0>] [vmiter <scalar:vmiter0>] [hmiter <scalar:hmiter0>],\n"
            "<vector:p1> [ext <scalar:ext1>] [vmiter <scalar:vmiter1>] [hmiter <scalar:hmiter1>],\n"
            "<feature:xsec>, <scalar:thickness>, <vector:up> )",

            _("This command creates a straight extruded volume from a planar section."
              "The section is expected comprise only of lines and the resulting bar will be of sheets with some added thicknes."
              "The sheets are accessible as subshapes for possible use in e.g. FEM models."
              "\n"
              "The location of the bar is specified by its start point p0 and end point p1."
              " Also needed is a face feature xsec defining the cross section."
              " For orienting the cross section an upward direction up has to be given."
              "\n"
              "The end points can optionally be shifted along the bar axis by offsets ext0 and ext1."
              " Furthermore, the bar ends can be mitered around the vertical axis (vmiter0, vmiter1) and the horizontal axis (hmiter0, hmiter1).")
            )
    };
}


}
}
