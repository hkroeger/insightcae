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

#include "dxfwriter.h"

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
: isleaf_(true)
{
}

SolidModel::SolidModel(const SolidModel& o)
: isleaf_(true), shape_(o.shape_)
{
  nameFeatures();
  cout<<"Copied SolidModel"<<endl;
}

SolidModel::SolidModel(const TopoDS_Shape& shape)
: isleaf_(true), shape_(shape)
{
  nameFeatures();
}

SolidModel::SolidModel(const boost::filesystem::path& filepath)
: isleaf_(true), shape_(loadShapeFromFile(filepath))
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

arma::mat SolidModel::modelCoG() const
{
  GProp_GProps props;
  BRepGProp::LinearProperties(shape_, props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat SolidModel::modelBndBox(double deflection) const
{
  Bnd_Box boundingBox;
  BRepBndLib::Add(shape_, boundingBox);

  //   if (deflection>0)
//   {
// //     Bnd_Box boundingBox;
// //     BRepBndLib::Add(shape_, boundingBox);
// // 
// //     double aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
// //     boundingBox.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
// //     cout<<aXmin<<" "<<aYmin<<" "<<aZmin<<", "<<aXmax<<" "<<aYmax<<" "<<aZmax<<endl;
// //     double deflection= std::min( aXmax-aXmin , std::min(aYmax-aYmin , aZmax-aZmin))*0.001;  
//      BRepMesh_IncrementalMesh Inc(shape_, deflection);
//     
// //         BRepMesh_FastDiscret m(0.001, shape_, boundingBox, 0.1, true, true, true, true);
//         //m.Perform(shape_);
// 
//   }
  
  if (deflection>0)
  {
      BRepMesh_FastDiscret m(deflection, 0.5, boundingBox, true, true, false, true);
      m.Perform(shape_);
      //    BRepMesh_IncrementalMesh Inc(shape, deflection);
  }

  arma::mat x=arma::zeros(3,2);
  boundingBox.Get
  (
    x(0,0), x(1,0), x(2,0), 
    x(0,1), x(1,1), x(2,1)
  );

  return x;
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
  cout<<filename<<" >> "<<ext<<endl;
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

SolidModel::View SolidModel::createView
(
  const arma::mat p0,
  const arma::mat n,
  bool section
) const
{
  View result_view;
  
  TopoDS_Shape dispshape=shape_;
  
  gp_Pnt p_base = gp_Pnt(p0(0), p0(1), p0(2));
  gp_Dir view_dir = gp_Dir(n(0), n(1), n(2));
  
  gp_Ax2 viewCS(p_base, view_dir); 
  HLRAlgo_Projector projector( viewCS );
  gp_Trsf transform=projector.FullTransformation();

  if (section)
  {
    gp_Dir normal = -view_dir;
    gp_Pln plane = gp_Pln(p_base, normal);
    gp_Pnt refPnt = gp_Pnt(p_base.X()-normal.X(), p_base.Y()-normal.Y(), p_base.Z()-normal.Z());
    
    TopoDS_Face Face = BRepBuilderAPI_MakeFace(plane);
    TopoDS_Shape HalfSpace = BRepPrimAPI_MakeHalfSpace(Face,refPnt).Solid();
    
    TopoDS_Compound dispshapes, xsecs;
    BRep_Builder builder1, builder2;
    builder1.MakeCompound( dispshapes );
    builder2.MakeCompound( xsecs );
    int i=-1, j=0;
    for (TopExp_Explorer ex(shape_, TopAbs_SOLID); ex.More(); ex.Next())
    {
      i++;
      try
      {
	builder1.Add(dispshapes, 	BRepAlgoAPI_Cut(ex.Current(), HalfSpace));
	builder2.Add(xsecs, 		BRepBuilderAPI_Transform(BRepAlgoAPI_Common(ex.Current(), Face), transform).Shape());
	j++;
      }
      catch (...)
      {
	cout<<"Warning: Failed to compute cross section of solid #"<<i<<endl;
      }
    }
    cout<<"Generated "<<j<<" cross-sections"<<endl;
    dispshape=dispshapes;
    result_view.crossSection = xsecs;
  }
  
  
  Handle_HLRBRep_Algo brep_hlr = new HLRBRep_Algo;
  brep_hlr->Add( dispshape );
  brep_hlr->Projector( projector );
  brep_hlr->Update();
  brep_hlr->Hide();

  // extracting the result sets:
  HLRBRep_HLRToShape shapes( brep_hlr );
  
  TopoDS_Compound allVisible;
  BRep_Builder builder;
  builder.MakeCompound( allVisible );
  TopoDS_Shape vs=shapes.VCompound();
  if (!vs.IsNull()) builder.Add(allVisible, vs);
  TopoDS_Shape r1vs=shapes.Rg1LineVCompound();
  if (!r1vs.IsNull()) builder.Add(allVisible, r1vs);
  TopoDS_Shape olvs = shapes.OutLineVCompound();
  if (!olvs.IsNull()) builder.Add(allVisible, olvs);
  
  TopoDS_Shape HiddenEdges = shapes.HCompound();
  
  result_view.visibleEdges=allVisible;
  result_view.hiddenEdges=HiddenEdges;
  
  return result_view;
  
//   BRepTools::Write(allVisible, "visible.brep");
//   BRepTools::Write(HiddenEdges, "hidden.brep");
//   
//   {
//     std::vector<LayerDefinition> addlayers;
//     if (section) addlayers.push_back
//     (
//       LayerDefinition("section", DL_Attributes(std::string(""), DL_Codes::black, 35, "CONTINUOUS"), false)
//     );
//     
//     DXFWriter dxf("view.dxf", addlayers);
//     
//     dxf.writeShapeEdges(allVisible, "0");
//     dxf.writeShapeEdges(HiddenEdges, "0_HL");
//     
//     if (!secshape.IsNull())
//     {
//       BRepTools::Write(secshape, "section.brep");
//       dxf.writeSection( BRepBuilderAPI_Transform(secshape, transform).Shape(), "section");
//     }
//   }
  
}

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
  const arma::mat& L3,
  bool centered
)
{
  Handle_Geom_Plane pln=GC_MakePlane(to_Pnt(p0), to_Pnt(p0+L1), to_Pnt(p0+L2)).Value();
  TopoDS_Shape box=
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
  if (centered)
  {
    gp_Trsf t;
    t.SetTranslation(to_Vec(-0.5*L1-0.5*L2-0.5*L3));
    box=BRepBuilderAPI_Transform(box, t).Shape();
  }
  return box;
}
  
Box::Box
(
  const arma::mat& p0, 
  const arma::mat& L1, 
  const arma::mat& L2, 
  const arma::mat& L3,
  bool centered
)
: SolidModel(makeBox(p0, L1, L2, L3, centered))
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

Extrusion::Extrusion(const SolidModel& sk, const arma::mat& L)
: SolidModel
(
  BRepPrimAPI_MakePrism( TopoDS::Face(sk), to_Vec(L) ).Shape()
)
{
}

BooleanUnion::BooleanUnion(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Fuse(m1, m2).Shape())
{
  m1.unsetLeaf();
  m2.unsetLeaf();
  cout<<"Union done"<<endl;
}

SolidModel operator|(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanUnion(m1, m2);
}


BooleanSubtract::BooleanSubtract(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Cut(m1, m2).Shape())
{
  m1.unsetLeaf();
  m2.unsetLeaf();
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
  m1.unsetLeaf();
}

TopoDS_Shape Chamfer::makeChamfers(const SolidModel& m1, const FeatureSet& edges, double l)
{
  BRepFilletAPI_MakeChamfer fb(m1);
  BOOST_FOREACH(FeatureID f, edges)
  {
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(m1, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    fb.Add(l, m1.edge(f), TopoDS::Face(mapEdgeFace(f).First()) );
  }
  fb.Build();
  return fb.Shape();
}
  
Chamfer::Chamfer(const SolidModel& m1, const FeatureSet& edges, double l)
: SolidModel(makeChamfers(m1, edges, l))
{
  m1.unsetLeaf();
}


TopoDS_Shape CircularPattern::makePattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);
  
  double delta_phi=norm(axis, 2);
  gp_Ax1 ax(to_Pnt(p0), to_Vec(axis/delta_phi));
  for (int i=0; i<n; i++)
  {
    gp_Trsf tr;
    tr.SetRotation(ax, delta_phi*double(i));
    bb.Add(result, BRepBuilderAPI_Transform(m1, tr).Shape());
  }
  
  return result;
}
  
CircularPattern::CircularPattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n)
: SolidModel(makePattern(m1, p0, axis, n))
{
  m1.unsetLeaf();
}

TopoDS_Shape LinearPattern::makePattern(const SolidModel& m1, const arma::mat& axis, int n)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);
  
  double delta_x=norm(axis, 2);
  gp_Vec ax(to_Vec(axis/delta_x));
  
  for (int i=0; i<n; i++)
  {
    gp_Trsf tr;
    tr.SetTranslation(ax*delta_x*double(i));
    bb.Add(result, BRepBuilderAPI_Transform(m1, tr).Shape());
  }
  
  return result;
}
  
LinearPattern::LinearPattern(const SolidModel& m1, const arma::mat& axis, int n)
: SolidModel(makePattern(m1, axis, n))
{
  m1.unsetLeaf();
}

TopoDS_Shape Transform::makeTransform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot)
{
  gp_Trsf tr1, tr2;
  TopoDS_Shape intermediate_shape=m1;

  tr1.SetTranslation(to_Vec(trans));  
  intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr1).Shape();

  double phi=norm(rot, 2);
  if (phi>1e-10)
  {
    gp_Vec axis=to_Vec(rot);
    axis.Normalize();
    tr2.SetRotation(gp_Ax1(gp_Pnt(0,0,0), axis), phi);
    intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr2).Shape();
  }  

  // Apply rotation first, then translation
  return intermediate_shape;
}

TopoDS_Shape Transform::makeTransform(const SolidModel& m1, const gp_Trsf& trsf)
{
  return BRepBuilderAPI_Transform(m1, trsf).Shape();
}


Transform::Transform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot)
: SolidModel(makeTransform(m1, trans, rot))
{
  m1.unsetLeaf();
}

Transform::Transform(const SolidModel& m1, const gp_Trsf& trsf)
: SolidModel(makeTransform(m1, trsf))
{
  m1.unsetLeaf();
}

TopoDS_Shape Compound::makeCompound(const std::vector<SolidModel::Ptr>& m1)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);
  
  BOOST_FOREACH(const SolidModel::Ptr& p, m1)
  {
    bb.Add(result, *p);
  }
  
  return result;
}


Compound::Compound(const std::vector<SolidModel::Ptr>& m1)
: SolidModel(makeCompound(m1))
{
  BOOST_FOREACH(const SolidModel::Ptr& p, m1)
  {
    p->unsetLeaf();
  }
}

}
}