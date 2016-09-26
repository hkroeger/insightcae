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
#include "quad.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

//using namespace std;
//using namespace boost;

namespace insight {
namespace cad {

  
defineType(Bar);
addToFactoryTable(Feature, Bar, NoParameters);




void Bar::build()
{

    if (!cache.contains(hash()))
    {

        arma::mat p0=*p0_;
        arma::mat p1=*p1_;

        if (norm(vert_->value(),2)<1e-10)
            throw insight::Exception("Bar: length of vertical direction is zero!");
        arma::mat v=vert_->value() / norm(vert_->value(),2);
        /*
              if (!xsec_->isSingleFace() || xsec_->isSingleWire() || xsec_->isSingleEdge())
        	throw insight::Exception("xsec feature has to provide a face or wire!");
          */
        arma::mat baraxis=p1-p0;
        double lba=norm(baraxis,2);
        if (lba<1e-10)
            throw insight::Exception("Bar: invalid definition of bar end points!");
        baraxis/=lba;

        p0 += -baraxis*(*ext0_);
        p1 +=  baraxis*(*ext1_);
        double L=norm(p1-p0, 2);

        refpoints_["start"]=*p0_;
        refpoints_["end"]=*p1_;
        refpoints_["p0"]=p0;
        refpoints_["p1"]=p1;

        refvalues_["L"]=L;

        TopoDS_Wire spine=BRepBuilderAPI_MakeWire
                          (
                              BRepBuilderAPI_MakeEdge
                              (
                                  GC_MakeSegment(to_Pnt(p0), to_Pnt(p1))
                              )
                          );
        //   TopoDS_Vertex pfirst, plast;
        //   TopExp::Vertices( spine, pfirst, plast );


        arma::mat ex=-arma::cross(baraxis, vert_->value());

        double lex=norm(ex, 2);
        if (lex<1e-10)
            throw insight::Exception("Bar: invalid definition of vertical direction!");
        ex/=lex;

        arma::mat ey=arma::cross(baraxis, ex);

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
        TopoDS_Shape xsecs =
            BRepBuilderAPI_Transform
            (
                xsec_->shape(),
                tr.Inverted()
            ).Shape();

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
                             matconst(p0_->value() -0.5*L*(cex+cey)),
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
                             matconst(p1_->value() -0.5*L*(cex+cey)),
                             matconst(L*cex),
                             matconst(L*cey)
                         );
            TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q->shape()), to_Vec(L*baraxis) );
            result=BRepAlgoAPI_Cut(result, airspace);
        }

        setShape(result);

//         std::cout<<"bar"<<std::endl;
        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<Bar>(hash()));
    }
}




Bar::Bar(const NoParameters& nop)
: Feature(nop)
{}




Bar::Bar
(
  VectorPtr start, VectorPtr end, 
  FeaturePtr xsec, VectorPtr vert, 
  ScalarPtr ext0, ScalarPtr ext1,
  ScalarPtr miterangle0_vert, ScalarPtr miterangle1_vert,
  ScalarPtr miterangle0_hor, ScalarPtr miterangle1_hor
)
: p0_(start),
  p1_(end),
  xsec_(xsec),
  vert_(vert),
  ext0_(ext0),
  ext1_(ext1),
  miterangle0_vert_(miterangle0_vert),
  miterangle1_vert_(miterangle1_vert),
  miterangle0_hor_(miterangle0_hor),
  miterangle1_hor_(miterangle1_hor)
{
  // build the parameter hash
  ParameterListHash h(this);
  h+=p0_->value();
  h+=p1_->value();
  h+=*xsec_;
  h+=vert_->value();
  h+=ext0_->value();
  h+=ext1_->value();
  h+=miterangle0_vert_->value();
  h+=miterangle1_vert_->value();
  h+=miterangle0_hor_->value();
  h+=miterangle1_hor_->value();
}




Bar::Bar
(
  VectorPtr start, VectorPtr end, 
  FeaturePtr xsec, VectorPtr vert, 
  const boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>& ext_miterv_miterh0, 
  const boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>& ext_miterv_miterh1
)
: p0_(start),
  p1_(end),
  xsec_(xsec),
  vert_(vert),
  ext0_(boost::fusion::at_c<0>(ext_miterv_miterh0)),
  ext1_(boost::fusion::at_c<0>(ext_miterv_miterh1)),
  miterangle0_vert_(boost::fusion::at_c<1>(ext_miterv_miterh0)),
  miterangle1_vert_(boost::fusion::at_c<1>(ext_miterv_miterh1)),
  miterangle0_hor_(boost::fusion::at_c<2>(ext_miterv_miterh0)),
  miterangle1_hor_(boost::fusion::at_c<2>(ext_miterv_miterh1))
{
  // build the parameter hash
  ParameterListHash h(this);
  h+=p0_->value();
  h+=p1_->value();
  h+=*xsec_;
  h+=vert_->value();
  h+=ext0_->value();
  h+=ext1_->value();
  h+=miterangle0_vert_->value();
  h+=miterangle1_vert_->value();
  h+=miterangle0_hor_->value();
  h+=miterangle1_hor_->value();
}




FeaturePtr Bar::create
(
    VectorPtr p0, VectorPtr p1,
    FeaturePtr xsec, VectorPtr vert,
    ScalarPtr ext0, ScalarPtr ext1,
    ScalarPtr miterangle0_vert, ScalarPtr miterangle1_vert,
    ScalarPtr miterangle0_hor, ScalarPtr miterangle1_hor
)
{
    return FeaturePtr
           (
               new Bar
               (
                   p0, p1,
                   xsec, vert,
                   ext0, ext1,
                   miterangle0_vert, miterangle1_vert,
                   miterangle0_hor, miterangle1_hor
               )
           );
}




FeaturePtr Bar::create
(
    VectorPtr p0, VectorPtr p1,
    FeaturePtr xsec, VectorPtr vert,
    const boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>& ext_miterv_miterh0,
    const boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr>& ext_miterv_miterh1
)
{
    return FeaturePtr
           (
               new Bar
               (
                   p0, p1,
                   xsec, vert,
                   ext_miterv_miterh0,
                   ext_miterv_miterh1
               )
           );
}




void Bar::operator=(const Bar& o)
{
  p0_=o.p0_;
  p1_=o.p1_;
  xsec_=o.xsec_;
  vert_=o.vert_;
  ext0_=o.ext0_;
  ext1_=o.ext1_;
  miterangle0_vert_=o.miterangle0_vert_;
  miterangle1_vert_=o.miterangle1_vert_;
  miterangle0_hor_=o.miterangle0_hor_;
  miterangle1_hor_=o.miterangle1_hor_;
  Feature::operator=(o);
}




/**
 * \page iscad_bar Bar
 *
 * The "Bar" command creates a straight extruded volume. 
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


void Bar::insertrule(parser::ISCADParser& ruleset) const
{
    ruleset.modelstepFunctionRules.add
    (
        "Bar",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

                    ( '('
                      >> ruleset.r_vectorExpression // 1
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
                      >> ruleset.r_vectorExpression >> // 6
                      ')' )
//                     [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Bar>
//                                  (
//                                      qi::_1, qi::_3,
//                                      qi::_5, qi::_6,
//                                      qi::_2, qi::_4
//                                  )) ]
                    [ qi::_val = phx::bind(&Bar::create, 
                                     qi::_1, qi::_3,
                                     qi::_5, qi::_6,
                                     qi::_2, qi::_4
                                 ) ]

                ))
    );
}

}
}
