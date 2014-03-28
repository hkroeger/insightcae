/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>
#include "solidmodel.h"
#include <base/exception.h>
#include "boost/foreach.hpp"
#include "geotest.h"

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

ANDFilter::ANDFilter(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}
void ANDFilter::initialize(const SolidModel& m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

bool ANDFilter::checkMatch(FeatureID feature) const
{
  return f1_->checkMatch(feature) && f2_->checkMatch(feature);
}

Filter* ANDFilter::clone() const
{
  return new ANDFilter(*f1_, *f2_);
}

NOTFilter::NOTFilter(const Filter& f1)
: Filter(),
  f1_(f1.clone())
{
}
void NOTFilter::initialize(const SolidModel& m)
{
  f1_->initialize(m);
}

bool NOTFilter::checkMatch(FeatureID feature) const
{
  return !f1_->checkMatch(feature);
}

Filter* NOTFilter::clone() const
{
  return new NOTFilter(*f1_);
}

ANDFilter operator&&(const Filter& f1, const Filter& f2)
{
  return ANDFilter(f1, f2);
}

NOTFilter operator!(const Filter& f1)
{
  return NOTFilter(f1);
}


edgeTopology::edgeTopology(GeomAbs_CurveType ct)
: ct_(ct)
{
}

bool edgeTopology::checkMatch(FeatureID feature) const
{
  return model_->edgeType(feature) == ct_;
}

Filter* edgeTopology::clone() const
{
  return new edgeTopology(ct_);
}

everything::everything()
{}

bool everything::checkMatch(FeatureID feature) const
{
  return true;
}
  
Filter* everything::clone() const
{
  return new everything();
}
  
coincides::coincides(const SolidModel& m, FeatureSet f, EntityType et)
: m_(m),
  f_(f),
  et_(et)
{
}

bool coincides::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  switch (et_)
  {
    case Edge:
      BOOST_FOREACH(int f, f_)
      {
	TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
	TopoDS_Edge e2=TopoDS::Edge(m_.edge(f));
	match |= isPartOf(e2, e1);
      }
      return match;
      break;
    default:
      throw insight::Exception("Filter coincides: Cannot handle entity type!");
  }
  
  return false;
}

Filter* coincides::clone() const
{
  return new coincides(m_, f_);
}
  
SolidModel::SolidModel(const SolidModel& o)
: shape_(o.shape_)
{
  nameFeatures();
}

SolidModel::SolidModel(const TopoDS_Shape& shape)
: shape_(shape)
{
  nameFeatures();
}

SolidModel::~SolidModel()
{
}

GeomAbs_CurveType SolidModel::edgeType(FeatureID i) const
{
  const TopoDS_Edge& e = edge(i);
  double t0, t1;
  Handle_Geom_Curve crv=BRep_Tool::Curve(e, t0, t1);
  GeomAdaptor_Curve adapt(crv);
  return adapt.GetType();
}

arma::mat SolidModel::edgeCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::LinearProperties(edge(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

FeatureSet SolidModel::query_edges(const Filter& filter) const
{
  std::auto_ptr<Filter> f(filter.clone());
  
  f->initialize(*this);
  FeatureSet res;
  for (int i=1; i<=emap_.Extent(); i++)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  return res;
}

void SolidModel::saveAs(const boost::filesystem::path& filename) const
{
  std::string ext=filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  if (ext==".brep")
  {
    BRepTools::Write(shape_, filename.c_str());
  } 
  else if ( (ext==".igs") || (ext==".iges") )
  {
    Interface_Static::SetIVal("write.iges.brep.mode", 1);
    IGESControl_Controller igesctrl;
    igesctrl.Init();
    IGESControl_Writer igeswriter;
    igeswriter.AddShape(shape_);
    igeswriter.Write(filename.c_str());
  } 
  else if ( (ext==".stp") || (ext==".step") )
  {
    STEPControl_Writer stepwriter;
    stepwriter.Transfer(shape_, STEPControl_AsIs);
    stepwriter.Write(filename.c_str());
  } 
  else if ( (ext==".stl") || (ext==".stlb") )
  {
    StlAPI_Writer stlwriter;

    stlwriter.ASCIIMode() = (ext==".stl");
    //stlwriter.RelativeMode()=false;
    //stlwriter.SetDeflection(maxdefl);
    stlwriter.SetCoefficient(5e-5);
    stlwriter.Write(shape_, filename.c_str());
  }
  else
  {
    throw insight::Exception("Unknown export file format! (Extension "+ext+")");
  }
}

SolidModel::operator const TopoDS_Shape& () const 
{ return shape_; }

edgeCoG::edgeCoG() 
{}

edgeCoG::~edgeCoG()
{}
  
arma::mat edgeCoG::evaluate(FeatureID ei)
{
  return model_->edgeCoG(ei);
}
  
QuantityComputer<arma::mat>* edgeCoG::clone() const 
{
  return new edgeCoG();
}
  
void SolidModel::nameFeatures()
{

    // Solids
    TopExp_Explorer exp0, exp1, exp2, exp3, exp4, exp5;
    for(exp0.Init(shape_, TopAbs_SOLID); exp0.More(); exp0.Next()) {
        TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
        if(somap_.FindIndex(solid) < 1) {
            somap_.Add(solid);

            for(exp1.Init(solid, TopAbs_SHELL); exp1.More(); exp1.Next()) {
                TopoDS_Shell shell = TopoDS::Shell(exp1.Current());
                if(shmap_.FindIndex(shell) < 1) {
                    shmap_.Add(shell);

                    for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
                        TopoDS_Face face = TopoDS::Face(exp2.Current());
                        if(fmap_.FindIndex(face) < 1) {
                            fmap_.Add(face);

                            for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
                                TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                                if(wmap_.FindIndex(wire) < 1) {
                                    wmap_.Add(wire);

                                    for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                                        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                                        if(emap_.FindIndex(edge) < 1) {
                                            emap_.Add(edge);

                                            for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                                                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                                                if(vmap_.FindIndex(vertex) < 1)
                                                    vmap_.Add(vertex);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Free Shells
    for(exp1.Init(exp0.Current(), TopAbs_SHELL, TopAbs_SOLID); exp1.More(); exp1.Next()) {
        TopoDS_Shape shell = exp1.Current();
        if(shmap_.FindIndex(shell) < 1) {
            shmap_.Add(shell);

            for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
                TopoDS_Face face = TopoDS::Face(exp2.Current());
                if(fmap_.FindIndex(face) < 1) {
                    fmap_.Add(face);

                    for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
                        TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                        if(wmap_.FindIndex(wire) < 1) {
                            wmap_.Add(wire);

                            for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                                TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                                if(emap_.FindIndex(edge) < 1) {
                                    emap_.Add(edge);

                                    for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                                        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                                        if(vmap_.FindIndex(vertex) < 1)
                                            vmap_.Add(vertex);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Free Faces
    for(exp2.Init(shape_, TopAbs_FACE, TopAbs_SHELL); exp2.More(); exp2.Next()) {
        TopoDS_Face face = TopoDS::Face(exp2.Current());
        if(fmap_.FindIndex(face) < 1) {
            fmap_.Add(face);

            for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
                TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                if(wmap_.FindIndex(wire) < 1) {
                    wmap_.Add(wire);

                    for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                        if(emap_.FindIndex(edge) < 1) {
                            emap_.Add(edge);

                            for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                                if(vmap_.FindIndex(vertex) < 1)
                                    vmap_.Add(vertex);
                            }
                        }
                    }
                }
            }
        }
    }

    // Free Wires
    for(exp3.Init(shape_, TopAbs_WIRE, TopAbs_FACE); exp3.More(); exp3.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
        if(wmap_.FindIndex(wire) < 1) {
            wmap_.Add(wire);

            for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                if(emap_.FindIndex(edge) < 1) {
                    emap_.Add(edge);

                    for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                        if(vmap_.FindIndex(vertex) < 1)
                            vmap_.Add(vertex);
                    }
                }
            }
        }
    }

    // Free Edges
    for(exp4.Init(shape_, TopAbs_EDGE, TopAbs_WIRE); exp4.More(); exp4.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
        if(emap_.FindIndex(edge) < 1) {
            emap_.Add(edge);

            for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                if(vmap_.FindIndex(vertex) < 1)
                    vmap_.Add(vertex);
            }
        }
    }

    // Free Vertices
    for(exp5.Init(shape_, TopAbs_VERTEX, TopAbs_EDGE); exp5.More(); exp5.Next()) {
        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
        if(vmap_.FindIndex(vertex) < 1)
            vmap_.Add(vertex);
    }

}

Cylinder::Cylinder(const arma::mat& p1, const arma::mat& p2, double D)
: SolidModel
  (
    BRepPrimAPI_MakeCylinder
    (
      gp_Ax2
      (
	gp_Pnt(p1(0),p1(1),p1(2)), 
	gp_Dir(p2(0)-p1(0),p2(1)-p1(1),p2(2)-p1(2))
      ),
      0.5*D, 
      norm(p2-p1, 2)
    ).Shape()
  )
{
}

TopoDS_Shape Box::makeBox
(
  const arma::mat& p0, 
  const arma::mat& L1, 
  const arma::mat& L2, 
  const arma::mat& L3
)
{
  Handle_Geom_Plane pln=GC_MakePlane(to_Pnt(p0), to_Pnt(p0+L1), to_Pnt(p0+L2)).Value();
  return 
  BRepPrimAPI_MakePrism
  (
    BRepBuilderAPI_MakeFace
    (
      pln,
      BRepBuilderAPI_MakePolygon
      (
	to_Pnt(p0), 
	to_Pnt(p0+L1), 
	to_Pnt(p0+L1+L2), 
	to_Pnt(p0+L2), 
	true
      ).Wire()
    ).Face(),
    to_Vec(L3)
  ).Shape();
}
  
Box::Box
(
  const arma::mat& p0, 
  const arma::mat& L1, 
  const arma::mat& L2, 
  const arma::mat& L3
)
: SolidModel(makeBox(p0, L1, L2, L3))
{
}
  
Sphere::Sphere(const arma::mat& p, double D)
: SolidModel
  (
    BRepPrimAPI_MakeSphere
    (
      gp_Pnt(p(0),p(1),p(2)),
      0.5*D
    ).Shape()
  )
{
}

BooleanUnion::BooleanUnion(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Fuse(m1, m2).Shape())
{
}

SolidModel operator|(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanUnion(m1, m2);
}


BooleanSubtract::BooleanSubtract(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Cut(m1, m2).Shape())
{
}

SolidModel operator-(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanSubtract(m1, m2);
}

}
}