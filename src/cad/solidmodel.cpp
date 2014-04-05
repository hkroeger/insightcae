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

#include "geotest.h"
#include "sketch.h"

#include <memory>
#include "solidmodel.h"
#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>

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

std::ostream& operator<<(std::ostream& os, const SolidModel& m)
{
  os<<"ENTITIES\n================\n\n";
  BRepTools::Dump(m.shape_, os);
  os<<"\n================\n\n";
  return os;
}

TopoDS_Shape SolidModel::loadShapeFromFile(const boost::filesystem::path& filename)
{
  cout<<"Reading "<<filename<<endl;
    
  std::string ext=filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  if (ext==".brep")
  {
    BRep_Builder bb;
    TopoDS_Shape s;
    BRepTools::Read(s, filename.c_str(), bb);
    return s;
  } 
  else if ( (ext==".igs") || (ext==".iges") )
  {
    IGESControl_Reader igesReader;

    igesReader = IGESControl_Reader();
    igesReader.ReadFile(filename.c_str());
    igesReader.TransferRoots();

    return igesReader.OneShape();
  } 
  else if ( (ext==".stp") || (ext==".step") )
  {
    STEPControl_Reader stepReader;

    stepReader = STEPControl_Reader();
    stepReader.ReadFile(filename.c_str());
    stepReader.TransferRoots();

    return stepReader.OneShape();
  } 
  else
  {
    throw insight::Exception("Unknown import file format! (Extension "+ext+")");
    return TopoDS_Shape();
  }  
}

SolidModel::SolidModel()
{
}

SolidModel::SolidModel(const SolidModel& o)
: shape_(o.shape_)
{
  nameFeatures();
  cout<<"Copied SolidModel"<<endl;
}

SolidModel::SolidModel(const TopoDS_Shape& shape)
: shape_(shape)
{
  nameFeatures();
}

SolidModel::SolidModel(const boost::filesystem::path& filepath)
: shape_(loadShapeFromFile(filepath))
{
  nameFeatures();
}

SolidModel::~SolidModel()
{
}

SolidModel& SolidModel::operator=(const SolidModel& o)
{
  shape_=o.shape_;
  nameFeatures();
  cout<<"Assigned SolidModel"<<endl;
  return *this;
}

GeomAbs_CurveType SolidModel::edgeType(FeatureID i) const
{
  const TopoDS_Edge& e = edge(i);
  double t0, t1;
  Handle_Geom_Curve crv=BRep_Tool::Curve(e, t0, t1);
  GeomAdaptor_Curve adapt(crv);
  return adapt.GetType();
}

GeomAbs_SurfaceType SolidModel::faceType(FeatureID i) const
{
  const TopoDS_Face& f = face(i);
  double t0, t1;
  Handle_Geom_Surface surf=BRep_Tool::Surface(f);
  GeomAdaptor_Surface adapt(surf);
  return adapt.GetType();
}

arma::mat SolidModel::edgeCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::LinearProperties(edge(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat SolidModel::faceCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::LinearProperties(face(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat SolidModel::faceNormal(FeatureID i) const
{
  BRepGProp_Face prop(face(i));
  double u1,u2,v1,v2;
  prop.Bounds(u1, u2, v1, v2);
  double u = (u1+u2)/2.;
  double v = (v1+v2)/2.;
  gp_Vec vec;
  gp_Pnt pnt;
  prop.Normal(u,v,pnt,vec);
  vec.Normalize();
  return insight::vec3( vec.X(), vec.Y(), vec.Z() );  
}

FeatureSet SolidModel::allEdges() const
{
  return FeatureSet(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( emap_.Extent()+1 ) 
  );
}

FeatureSet SolidModel::allFaces() const
{
  return FeatureSet(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( fmap_.Extent()+1 ) 
  );
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
  fmap_.Clear();
  emap_.Clear(); 
  vmap_.Clear(); 
  somap_.Clear(); 
  shmap_.Clear(); 
  wmap_.Clear();
  
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
  cout<<"Cylinder created"<<endl;
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

Extrusion::Extrusion(const Sketch& sk, const arma::mat& L)
: SolidModel
(
  BRepPrimAPI_MakePrism( sk, to_Vec(L) ).Shape()
)
{
}

BooleanUnion::BooleanUnion(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Fuse(m1, m2).Shape())
{
  cout<<"Union done"<<endl;
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


TopoDS_Shape Fillet::makeFillets(const SolidModel& m1, const FeatureSet& edges, double r)
{
  BRepFilletAPI_MakeFillet fb(m1);
  BOOST_FOREACH(FeatureID f, edges)
  {
    fb.Add(r, m1.edge(f));
  }
  fb.Build();
  return fb.Shape();
}
  
Fillet::Fillet(const SolidModel& m1, const FeatureSet& edges, double r)
: SolidModel(makeFillets(m1, edges, r))
{
}

}
}