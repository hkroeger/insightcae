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
#include "solidmodel.h"
#include "datum.h"
#include "sketch.h"

#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include "boost/make_shared.hpp"

#include "dxfwriter.h"
#include "featurefilter.h"
#include "gp_Cylinder.hxx"

#include <BRepLib_FindSurface.hxx>
#include <BRepCheck_Shell.hxx>
#include "BRepOffsetAPI_NormalProjection.hxx"

#include "openfoam/openfoamdict.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;



namespace insight 
{
namespace cad 
{


std::ostream& operator<<(std::ostream& os, const SolidModel& m)
{
  os<<"ENTITIES\n================\n\n";
  BRepTools::Dump(m.shape_, os);
  os<<"\n================\n\n";
  return os;
}

defineType(SolidModel);
defineFactoryTable(SolidModel, NoParameters);
addToFactoryTable(SolidModel, SolidModel, NoParameters);

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

void SolidModel::setShape(const TopoDS_Shape& shape)
{
  shape_=shape;
  nameFeatures();
}


SolidModel::SolidModel(const NoParameters&)
: isleaf_(true),
  density_(1.0),
  areaWeight_(0.0)
{
}

SolidModel::SolidModel(const SolidModel& o)
: isleaf_(true),
  providedSubshapes_(o.providedSubshapes_),
  providedDatums_(o.providedDatums_),
  density_(1.0),
  areaWeight_(0.0),
  explicitCoG_(o.explicitCoG_),
  explicitMass_(o.explicitMass_)
{
  setShape(o.shape_);
}

SolidModel::SolidModel(const TopoDS_Shape& shape)
: isleaf_(true),
  density_(1.0),
  areaWeight_(0.0)
{
  setShape(shape);
}

SolidModel::SolidModel(const boost::filesystem::path& filepath)
: isleaf_(true),
  density_(1.0),
  areaWeight_(0.0)
{
  setShape(loadShapeFromFile(filepath));
}

SolidModel::~SolidModel()
{
}

double SolidModel::mass() const
{
  if (explicitMass_)
  {
    cout<<"Explicit mass = "<<*explicitMass_<<endl;
    return *explicitMass_;
  }
  else
  {
    double mtot=density_*modelVolume()+areaWeight_*modelSurfaceArea();
    cout<<"Computed mass rho / V = "<<density_<<" / "<<modelVolume()
	<<", mf / A = "<<areaWeight_<<" / "<<modelSurfaceArea()
	<<", m = "<<mtot<<endl;
    return mtot;
  }
}

void SolidModel::setMassExplicitly(double m) 
{ 
  explicitMass_.reset(new double(m));
}

void SolidModel::setCoGExplicitly(const arma::mat& cog) 
{ 
  explicitCoG_.reset(new arma::mat(cog));
}

SolidModel& SolidModel::operator=(const SolidModel& o)
{
  setShape(o.shape_);
  explicitCoG_=o.explicitCoG_;
  explicitMass_=o.explicitMass_;
  return *this;
}

bool SolidModel::operator==(const SolidModel& o) const
{
  return shape_==o.shape_;
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

arma::mat SolidModel::vertexLocation(FeatureID i) const
{
  gp_Pnt cog=BRep_Tool::Pnt(vertex(i));
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
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
  BRepGProp::SurfaceProperties(face(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat SolidModel::modelCoG() const
{
  if (explicitCoG_)
  {
    return *explicitCoG_;
  }
  else
  {
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape_, props);
    gp_Pnt cog = props.CentreOfMass();
    return vec3(cog);
  }
}

double SolidModel::modelVolume() const
{
  TopExp_Explorer ex(shape_, TopAbs_SOLID);
  if (ex.More())
  {
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape_, props);
    return props.Mass();
  }
  else
  {
    return 0.;
  }
}

double SolidModel::modelSurfaceArea() const
{
  TopExp_Explorer ex(shape_, TopAbs_FACE);
  if (ex.More())
  {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(shape_, props);
    return props.Mass();
  }
  else
  {
    return 0.;
  }
}

double SolidModel::minDist(const arma::mat& p) const
{
  BRepExtrema_DistShapeShape dss
  (
    BRepBuilderAPI_MakeVertex(to_Pnt(p)).Vertex(), 
    shape_
  );
  
  if (!dss.Perform())
    throw insight::Exception("determination of minimum distance to point failed!");
  return dss.Value();
}

double SolidModel::maxVertexDist(const arma::mat& p) const
{
  double maxdist=0.;
  for (TopExp_Explorer ex(shape_, TopAbs_VERTEX); ex.More(); ex.Next())
  {
    TopoDS_Vertex v=TopoDS::Vertex(ex.Current());
    arma::mat vp=Vector(BRep_Tool::Pnt(v)).t();
    maxdist=std::max(maxdist, norm(p-vp,2));
  }
  return maxdist;
}

double SolidModel::maxDist(const arma::mat& p) const
{
  if (!isSingleFace())
    throw insight::Exception("max distance determination from anything else than single face shapes is currently not supported!");
  
  BRepExtrema_ExtPF epf
  ( 
    BRepBuilderAPI_MakeVertex(to_Pnt(p)).Vertex(), 
    TopoDS::Face(asSingleFace()), 
    Extrema_ExtFlag_MAX,
    Extrema_ExtAlgo_Tree
  );
  std::cout<<"Nb="<<epf.NbExt()<<std::endl;
  if (!epf.IsDone())
    throw insight::Exception("determination of maximum distance to point failed!");
  double maxdistsq=0.;
  for (int i=1; i<epf.NbExt(); i++)
  {
    maxdistsq=std::max(maxdistsq, epf.SquareDistance(i));
  }
  return sqrt(maxdistsq);
}

arma::mat SolidModel::modelBndBox(double deflection) const
{
  if (deflection>0)
  {
      BRepMesh_IncrementalMesh Inc(shape_, deflection);
  }

  Bnd_Box boundingBox;
  BRepBndLib::Add(shape_, boundingBox);

  arma::mat x=arma::zeros(3,2);
  double g=boundingBox.GetGap();
  cout<<"gap="<<g<<endl;
  boundingBox.Get
  (
    x(0,0), x(1,0), x(2,0), 
    x(0,1), x(1,1), x(2,1)
  );
  x.col(0)+=g;
  x.col(1)-=g;

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

FeatureSet SolidModel::allVertices() const
{
  FeatureSet f(*this, Vertex);
  f.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( vmap_.Extent()+1 ) 
  );
  return f;
}

FeatureSet SolidModel::allEdges() const
{
  FeatureSet f(*this, Edge);
  f.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( emap_.Extent()+1 ) 
  );
  return f;
}

FeatureSet SolidModel::allFaces() const
{
  FeatureSet f(*this, Edge);
  f.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( fmap_.Extent()+1 ) 
  );
  return f;
}

FeatureSet SolidModel::query_vertices(const FilterPtr& f) const
{
  return query_vertices_subset(allVertices(), f);
}

FeatureSet SolidModel::query_vertices(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_vertices(parseVertexFilterExpr(is, refs));
}


FeatureSet SolidModel::query_vertices_subset(const FeatureSet& fs, const FilterPtr& f) const
{
//   Filter::Ptr f(filter.clone());
  
  f->initialize(*this);
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSet res(*this, Vertex);
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_VERTICES RESULT = "<<res<<endl;
  return res;
}

FeatureSet SolidModel::query_vertices_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_vertices_subset(fs, parseVertexFilterExpr(is, refs));
}



FeatureSet SolidModel::query_edges(const FilterPtr& f) const
{
//   Filter::Ptr f(filter.clone());
  return query_edges_subset(allEdges(), f);
}

FeatureSet SolidModel::query_edges(const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_edges(parseEdgeFilterExpr(is, refs));
}

FeatureSet SolidModel::query_edges_subset(const FeatureSet& fs, const FilterPtr& f) const
{
  f->initialize(*this);
  //for (int i=1; i<=emap_.Extent(); i++)
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSet res(*this, Edge);
  //for (int i=1; i<=emap_.Extent(); i++)
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_EDGES RESULT = "<<res<<endl;
  return res;
}

FeatureSet SolidModel::query_edges_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_edges_subset(fs, parseEdgeFilterExpr(is, refs));
}

FeatureSet SolidModel::query_faces(const FilterPtr& f) const
{
  return query_faces_subset(allFaces(), f);
}

FeatureSet SolidModel::query_faces(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces(parseFaceFilterExpr(is, refs));
}


FeatureSet SolidModel::query_faces_subset(const FeatureSet& fs, const FilterPtr& f) const
{
//   Filter::Ptr f(filter.clone());
  
  f->initialize(*this);
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSet res(*this, Face);
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_FACES RESULT = "<<res<<endl;
  return res;
}

FeatureSet SolidModel::query_faces_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces_subset(fs, parseFaceFilterExpr(is, refs));
}


FeatureSet SolidModel::verticesOfEdge(const FeatureID& e) const
{
  FeatureSet vertices(*this, Vertex);
  vertices.insert(vmap_.FindIndex(TopExp::FirstVertex(edge(e))));
  vertices.insert(vmap_.FindIndex(TopExp::LastVertex(edge(e))));
  return vertices;
}

FeatureSet SolidModel::verticesOfEdges(const FeatureSet& es) const
{
  FeatureSet vertices(*this, Vertex);
  BOOST_FOREACH(FeatureID i, es)
  {
    FeatureSet j=verticesOfEdge(i);
    vertices.insert(j.begin(), j.end());
  }
  return vertices;
}

FeatureSet SolidModel::verticesOfFace(const FeatureID& f) const
{
  FeatureSet vertices(*this, Vertex);
  for (TopExp_Explorer ex(face(f), TopAbs_VERTEX); ex.More(); ex.Next())
  {
    vertices.insert(vmap_.FindIndex(TopoDS::Vertex(ex.Current())));
  }
  return vertices;
}

FeatureSet SolidModel::verticesOfFaces(const FeatureSet& fs) const
{
  FeatureSet vertices(*this, Vertex);
  BOOST_FOREACH(FeatureID i, fs)
  {
    FeatureSet j=verticesOfFace(i);
    vertices.insert(j.begin(), j.end());
  }
  return vertices;
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

void SolidModel::exportSTL(const boost::filesystem::path& filename, double abstol) const
{
  TopoDS_Shape os=shape_;
  
  ShapeFix_ShapeTolerance sf;
  sf.SetTolerance(os, abstol);
  
  BRepMesh_IncrementalMesh binc(os, abstol);
  
  StlAPI_Writer stlwriter;

  stlwriter.ASCIIMode() = false;
  stlwriter.RelativeMode()=false;
  stlwriter.SetDeflection(abstol);
  stlwriter.Write(shape_, filename.c_str());
}


void SolidModel::exportEMesh
(
  const boost::filesystem::path& filename, 
  const FeatureSet& fs, 
  double abstol,
  double maxlen
)
{
  if (fs.shape()!=Edge) 
    throw insight::Exception("Called with incompatible feature set!");
  
  std::vector<arma::mat> points;
  typedef std::pair<int, int> Edge;
  std::vector<Edge> edges;
  
  BOOST_FOREACH(const FeatureID& fi, fs)
  {
    
    TopoDS_Edge e=fs.model().edge(fi);
    if (!BRep_Tool::Degenerated(e))
    {
      std::cout<<"feature "<<fi<<std::endl;
//       BRepTools::Dump(e, std::cout);
      BRepAdaptor_Curve ac(e);
      GCPnts_QuasiUniformDeflection qud(ac, abstol);

      if (!qud.IsDone())
	throw insight::Exception("Discretization of curves into eMesh failed!");

      double clen=0.;
      arma::mat lp;
      std::set<int> splits;
      int iofs=points.size();
      for (int j=1; j<=qud.NbPoints(); j++)
      {
	gp_Pnt pp=ac.Value(qud.Parameter(j));

	arma::mat p=Vector(pp);
	if (j>1) clen+=norm(p-lp, 2);

	lp=p;
	if ((clen>maxlen) && (j>1)) 
	{
	  points.push_back(lp+0.5*(p-lp)); // insert a second point at same loc
	  points.push_back(p); // insert a second point at same loc
	  splits.insert(points.size()-1); 
	  clen=0.0; 
	}
	else
	{
	  points.push_back(p);
	}
      }
      
      for (int i=1; i<points.size()-iofs; i++)
      {
	int from=iofs+i-1;
	int to=iofs+i;
	if (splits.find(to)==splits.end())
	  edges.push_back(Edge(from,to));
      }
    }
  }
  
  std::ofstream f(filename.c_str());
  f<<"FoamFile {"<<endl
   <<" version     2.0;"<<endl
   <<" format      ascii;"<<endl
   <<" class       featureEdgeMesh;"<<endl
   <<" location    \"\";"<<endl
   <<" object      "<<filename.filename().string()<<";"<<endl
   <<"}"<<endl;
   
  f<<points.size()<<endl
   <<"("<<endl;
  BOOST_FOREACH(const arma::mat& p, points)
  {
    f<<OFDictData::to_OF(p)<<endl;
  }
  f<<")"<<endl;

  f<<edges.size()<<endl
   <<"("<<endl;
  BOOST_FOREACH(const Edge& e, edges)
  {
    f<<"("<<e.first<<" "<<e.second<<")"<<endl;
  }
  f<<")"<<endl;

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

void SolidModel::insertrule(parser::ISCADParser& ruleset) const
{
}

bool SolidModel::isSingleEdge() const
{
  return false;
}

bool SolidModel::isSingleFace() const
{
  return false;
}

bool SolidModel::isSingleOpenWire() const
{
  return false;
}

bool SolidModel::isSingleClosedWire() const
{
  return false;
}

bool SolidModel::isSingleWire() const
{
  return isSingleOpenWire() || isSingleClosedWire();
}


bool SolidModel::isSingleVolume() const
{
  return false;
}

TopoDS_Edge SolidModel::asSingleEdge() const
{
  if (!isSingleEdge())
    throw insight::Exception("Feature "+type()+" does not provide a single edge!");
  else
    return TopoDS::Edge(shape_);
}

TopoDS_Face SolidModel::asSingleFace() const
{
  if (!isSingleFace())
    throw insight::Exception("Feature "+type()+" does not provide a single face!");
  else
    return TopoDS::Face(shape_);
}

TopoDS_Wire SolidModel::asSingleOpenWire() const
{
  if (!isSingleOpenWire())
    throw insight::Exception("Feature "+type()+" does not provide a single open wire!");
  else
  {
    return asSingleWire();
  }
}

TopoDS_Wire SolidModel::asSingleClosedWire() const
{
  if (!isSingleClosedWire())
    throw insight::Exception("Feature "+type()+" does not provide a single closed wire!");
  else
  {
    return asSingleWire();
  }
}

TopoDS_Wire SolidModel::asSingleWire() const
{
  if (isSingleWire())
    return TopoDS::Wire(shape_);
}


TopoDS_Shape SolidModel::asSingleVolume() const
{
  if (!isSingleVolume())
    throw insight::Exception("Feature "+type()+" does not provide a single volume!");
  else
    return shape_;
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




defineType(Import);
addToFactoryTable(SolidModel, Import, NoParameters);

Import::Import(const NoParameters& nop): SolidModel(nop)
{}


Import::Import(const filesystem::path& filepath)
: SolidModel(filepath)
{}

void Import::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "import",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' > ruleset.r_path > ')' ) [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Import>(qi::_1)) ]
    ))
  );
}

defineType(Spring);
addToFactoryTable(SolidModel, Spring, NoParameters);

Spring::Spring(const NoParameters& nop)
: SolidModel(nop)
{
}


Spring::Spring(const arma::mat& p0, const arma::mat& p1, double d, double winds)
: SolidModel()
{
  double l=to_Pnt(p1).Distance(to_Pnt(p0).XYZ());
  Handle_Geom_CylindricalSurface aCylinder
  (
    new Geom_CylindricalSurface(gp_Ax3(to_Pnt(p0), gp_Dir(to_Pnt(p1).XYZ()-to_Pnt(p0).XYZ())), 0.5*d)
  );

  gp_Lin2d aLine2d(gp_Pnt2d(0.0, 0.0), gp_Dir2d(2.*M_PI, l/winds));

  double ll=sqrt(pow(l,2)+pow(2.0*M_PI*winds,2));
  Handle_Geom2d_TrimmedCurve aSegment = GCE2d_MakeSegment(aLine2d, 0.0, ll);
  TopoDS_Edge ec=BRepBuilderAPI_MakeEdge(aSegment, aCylinder, 0.0, ll).Edge();
    
  BRepBuilderAPI_MakeWire wb;
  wb.Add(ec);
  wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p0), BRep_Tool::Pnt(TopExp::FirstVertex(ec)))));
  wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p1), BRep_Tool::Pnt(TopExp::LastVertex(ec)))));
//   BOOST_FOREACH(const FeatureID& fi, edges)
//   {
//     wb.Add(edges.model().edge(fi));
//   }
  setShape(wb.Wire());
}

void Spring::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Spring",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Spring>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
    ))
  );
}


defineType(SplineCurve);
addToFactoryTable(SolidModel, SplineCurve, NoParameters);

SplineCurve::SplineCurve(const NoParameters& nop): SolidModel(nop)
{}


SplineCurve::SplineCurve(const std::vector< arma::mat >& pts)
{
  TColgp_Array1OfPnt pts_col(1, pts.size());
  for (int j=0; j<pts.size(); j++) pts_col.SetValue(j+1, to_Pnt(pts[j]));
  GeomAPI_PointsToBSpline splbuilder(pts_col);
  Handle_Geom_BSplineCurve crv=splbuilder.Curve();
  setShape(BRepBuilderAPI_MakeEdge(crv, crv->FirstParameter(), crv->LastParameter()));
}

void SplineCurve::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SplineCurve",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression % ',' >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<SplineCurve>(qi::_1)) ]
      
    ))
  );
}


defineType(Wire);
addToFactoryTable(SolidModel, Wire, NoParameters);

Wire::Wire(const NoParameters& nop): SolidModel(nop)
{

}


Wire::Wire(const FeatureSet& edges)
{
  BRepBuilderAPI_MakeWire wb;
  BOOST_FOREACH(const FeatureID& fi, edges)
  {
    wb.Add(edges.model().edge(fi));
  }
  setShape(wb.Wire());
}

void Wire::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Wire",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_edgeFeaturesExpression >> ')' ) 
	  [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Wire>(*qi::_1)) ]
      
    ))
  );
}

bool Wire::isSingleCloseWire() const
{
  return TopoDS::Wire(shape_).Closed();
}

bool Wire::isSingleOpenWire() const
{
  return !isSingleCloseWire();
}


defineType(Line);
addToFactoryTable(SolidModel, Line, NoParameters);


Line::Line(const NoParameters& nop)
: SolidModel(nop)
{
}

Line::Line(const arma::mat& p0, const arma::mat& p1)
: SolidModel()
{
  setShape(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p0), to_Pnt(p1))));
}

void Line::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Line",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Line>(qi::_1, qi::_2)) ]
      
    ))
  );
}

bool Line::isSingleCloseWire() const
{
  return false;
}

bool Line::isSingleOpenWire() const
{
  return true;
}


defineType(Arc);
addToFactoryTable(SolidModel, Arc, NoParameters);



Arc::Arc(const NoParameters& nop)
: SolidModel(nop)
{

}

Arc::Arc(const arma::mat& p0, const arma::mat& p0tang, const arma::mat& p1)
: SolidModel()
{
  setShape(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p0), to_Vec(p0tang), to_Pnt(p1))));
}

void Arc::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Arc",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Arc>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

bool Arc::isSingleCloseWire() const
{
  return false;
}

bool Arc::isSingleOpenWire() const
{
  return true;
}



bool SingleFaceFeature::isSingleFace() const
{
  return true;
}

bool SingleFaceFeature::isSingleCloseWire() const
{
  return true;
}

TopoDS_Wire SingleFaceFeature::asSingleClosedWire() const
{
  return BRepTools::OuterWire(TopoDS::Face(shape_));;
}



defineType(Tri);
addToFactoryTable(SolidModel, Tri, NoParameters);

Tri::Tri(const NoParameters&)
{}

Tri::Tri(const arma::mat& p0, const arma::mat& e1, const arma::mat& e2)
{
  gp_Pnt 
    p1(to_Pnt(p0)),
    p2=p1.Translated(to_Vec(e1)),
    p3=p1.Translated(to_Vec(e2))
  ;
  
  BRepBuilderAPI_MakeWire w;
  w.Add(BRepBuilderAPI_MakeEdge(p1, p2));
  w.Add(BRepBuilderAPI_MakeEdge(p2, p3));
  w.Add(BRepBuilderAPI_MakeEdge(p3, p1));
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_.add("OuterWire", SolidModelPtr(new SolidModel(w.Wire())));
  
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}

Tri::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}

void Tri::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Tri",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Tri>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(Quad);
addToFactoryTable(SolidModel, Quad, NoParameters);

Quad::Quad(const NoParameters&)
{}


Quad::Quad(const arma::mat& p0, const arma::mat& L, const arma::mat& W)
{
  gp_Pnt 
    p1(to_Pnt(p0)),
    p2=p1.Translated(to_Vec(W)),
    p3=p2.Translated(to_Vec(L)),
    p4=p1.Translated(to_Vec(L))
  ;
  
  BRepBuilderAPI_MakeWire w;
  w.Add(BRepBuilderAPI_MakeEdge(p1, p2));
  w.Add(BRepBuilderAPI_MakeEdge(p2, p3));
  w.Add(BRepBuilderAPI_MakeEdge(p3, p4));
  w.Add(BRepBuilderAPI_MakeEdge(p4, p1));
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_.add("OuterWire", SolidModelPtr(new SolidModel(w.Wire())));
  
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}

Quad::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}

void Quad::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Quad",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Quad>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

defineType(Circle);
addToFactoryTable(SolidModel, Circle, NoParameters);

Circle::Circle(const NoParameters&)
{
}


TopoDS_Face makeCircle(const arma::mat& p0, const arma::mat& n, double D)
{
  Handle_Geom_Curve c=GC_MakeCircle(to_Pnt(p0), to_Vec(n), 0.5*D);
  
  BRepBuilderAPI_MakeWire w;
  w.Add(BRepBuilderAPI_MakeEdge(c));
  return BRepBuilderAPI_MakeFace(w.Wire());
}

Circle::Circle(const arma::mat& p0, const arma::mat& n, double D)
{
  setShape(makeCircle(p0, n, D));
}

Circle::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}

void Circle::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Circle",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Circle>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

defineType(RegPoly);
addToFactoryTable(SolidModel, RegPoly, NoParameters);

RegPoly::RegPoly(const NoParameters&)
{}


RegPoly::RegPoly(const arma::mat& p0, const arma::mat& n, double ne, double a, 
		 const arma::mat& ez)
{
  double ru=a/::cos(M_PI/ne);
  arma::mat e0=ez;
  if (e0.n_elem==0)
  {
    e0=cross(n, vec3(1,0,0));
    if (norm(e0,2)<1e-6) 
      e0=cross(n, vec3(0,1,0));
  }
  int z=round(ne);
  double delta_phi=2.*M_PI/double(z);
  BRepBuilderAPI_MakeWire w;
  for (int i=0; i<z; i++)
  {
    arma::mat npm=p0+rotMatrix((0.5+double(i-1))*delta_phi, n)*(ru*e0);
    arma::mat np=p0+rotMatrix((0.5+double(i))*delta_phi, n)*(ru*e0);
    w.Add(BRepBuilderAPI_MakeEdge(to_Pnt(npm), to_Pnt(np)));
  }
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_.add("OuterWire", SolidModelPtr(new SolidModel(w.Wire())));
  
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}

void RegPoly::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "RegPoly",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression 
			      > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression 
			      > ( (',' > ruleset.r_vectorExpression)|qi::attr(arma::mat()) ) > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<RegPoly>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}

defineType(SplineSurface);
addToFactoryTable(SolidModel, SplineSurface, NoParameters);

SplineSurface::SplineSurface(const NoParameters&)
{}


SplineSurface::SplineSurface(const std::vector< std::vector< arma::mat> >& pts)
{
  int nx=pts.size();
  if (nx<2)
    throw insight::Exception("SplineSurface: not enough rows of point specified!");
  int ny=pts[0].size();
  if (ny<2)
    throw insight::Exception("SplineSurface: not enough cols of point specified!");
  
  TColgp_Array2OfPnt pts_col(1, nx, 1, ny);
  for (int j=0; j<nx; j++) 
  {
    if (pts[j].size()!=ny)
      throw insight::Exception("SplineSurface: all rows need to have an equal number of points!");
    for (int k=0; k<ny; k++)
      pts_col.SetValue(j+1, k+1, to_Pnt(pts[j][k]));
  }
  GeomAPI_PointsToBSplineSurface spfbuilder(pts_col);
  Handle_Geom_BSplineSurface srf=spfbuilder.Surface();
  setShape(BRepBuilderAPI_MakeFace(srf, true));
}

SplineSurface::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}

void SplineSurface::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SplineSurface",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> 
	  ( ( '(' >> ( ruleset.r_vectorExpression % ',' ) >> ')' ) % ',' )
	  >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<SplineSurface>(qi::_1)) ]
      
    ))
  );
}


defineType(BoundedFlatFace);
addToFactoryTable(SolidModel, BoundedFlatFace, NoParameters);


BoundedFlatFace::BoundedFlatFace(const NoParameters&)
{}

BoundedFlatFace::BoundedFlatFace(const std::vector<SolidModelPtr>& edges)
{
  TopTools_ListOfShape edgs;
  
  int n_ok=0, n_nok=0;
  BOOST_FOREACH(const SolidModelPtr& m, edges)
  {
    if (m->isSingleEdge())
    {
      TopoDS_Edge e=m->asSingleEdge();
      edgs.Append(e);
      n_ok++;
    }
    else if (m->isSingleWire())
    {
      TopoDS_Wire wire=m->asSingleWire();
      for (TopExp_Explorer ex(wire, TopAbs_EDGE); ex.More(); ex.Next())
      {
	TopoDS_Edge e=TopoDS::Edge(ex.Current());
	edgs.Append(e);
      }
      n_ok++;
    }
    else n_nok++;
  }

  if (n_ok==0)
    throw insight::Exception("No valid edge given!");
  if (n_nok>0)
    insight::Warning(str(format("Only %d out of %d given edges were valid!") % n_ok % (n_ok+n_nok)));

  BRepBuilderAPI_MakeWire w;
  w.Add(edgs);
  
  BRepBuilderAPI_MakeFace fb(w.Wire(), true);
  if (!fb.IsDone())
    throw insight::Exception("Failed to generate planar face!");
  
  ShapeFix_Face FixShape;
  FixShape.Init(fb.Face());
  FixShape.Perform();
  
  setShape(FixShape.Face());
}

BoundedFlatFace::BoundedFlatFace(const std::vector<FeatureSetPtr>& edges)
{
  TopTools_ListOfShape edgs;
  BOOST_FOREACH(const FeatureSetPtr& m, edges)
  {
    BOOST_FOREACH(const FeatureID& fi, *m)
    {
      edgs.Append(m->model().edge(fi));
    }
  }
  
  BRepBuilderAPI_MakeWire w;
  w.Add(edgs);

  BRepBuilderAPI_MakeFace fb(w.Wire(), true);
  if (!fb.IsDone())
    throw insight::Exception("Failed to generate planar face!");

  ShapeFix_Face FixShape;
  FixShape.Init(fb.Face());
  FixShape.Perform();
  
  setShape(FixShape.Face());
}

BoundedFlatFace::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}


void BoundedFlatFace::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "BoundedFlatFace",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ( ruleset.r_solidmodel_expression % ',' ) >> ')' )
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<BoundedFlatFace>(qi::_1)) ]
    |
    ( '(' >> ( ruleset.r_edgeFeaturesExpression % ',' ) >> ')' )
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<BoundedFlatFace>(qi::_1)) ]
      
    ))
  );
}

defineType(FillingFace);
addToFactoryTable(SolidModel, FillingFace, NoParameters);


FillingFace::FillingFace(const NoParameters&)
{}

FillingFace::FillingFace(const SolidModel& e1, const SolidModel& e2)
{  
  TopoDS_Edge ee1, ee2;
  bool ok=true;
  if (e1.isSingleEdge())
  {
    ee1=e1.asSingleEdge();
  }
  else ok=false;
  if (e2.isSingleEdge())
  {
    ee2=e2.asSingleEdge();
  }
  else ok=false;

  if (!ok)
    throw insight::Exception("Invalid edge given!");

  TopoDS_Face f;
  try
  {
    f=BRepFill::Face(ee1, ee2);
  }
  catch (...)
  {
    throw insight::Exception("Failed to generate face!");
  }
  
  ShapeFix_Face FixShape;
  FixShape.Init(f);
  FixShape.Perform();
  
  setShape(FixShape.Face());
}

FillingFace::FillingFace(const FeatureSet& e1, const FeatureSet& e2)
{
  /*
  TopoDS_Edge ee1, ee2;
  {
    TopTools_ListOfShape edgs;
    BOOST_FOREACH(const FeatureID& i, e1)
    {
      edgs.Append( e1.model().edge(*e1.begin()) );
    }
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    Handle_Geom_Curve crv(new BRepAdaptor_CompCurve(w.Wire()));
    ee1=BRepBuilderAPI_MakeEdge(crv);
  }

  {
    TopTools_ListOfShape edgs;
    BOOST_FOREACH(const FeatureID& i, e1)
    {
      edgs.Append( e2.model().edge(*e2.begin()) );
    }
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    Handle_Geom_Curve crv(new BRepAdaptor_CompCurve(w.Wire()));
    ee2=BRepBuilderAPI_MakeEdge(crv);
  }
  */
  
  TopoDS_Edge ee1, ee2;
  if (e1.size()!=1)
  {
    throw insight::Exception("first feature set has to contain only 1 edge!");
  }
  else
  {
    ee1=e1.model().edge(*e1.begin());
  }
  
  if (e2.size()!=1)
  {
    throw insight::Exception("second feature set has to contain only 1 edge!");
  }
  else
  {
    ee2=e2.model().edge(*e2.begin());
  }

  TopoDS_Face f;
  try
  {
    f=BRepFill::Face(ee1, ee2);
  }
  catch (...)
  {
    throw insight::Exception("Failed to generate face!");
  }
  
  ShapeFix_Face FixShape;
  FixShape.Init(f);
  FixShape.Perform();
  
  setShape(FixShape.Face());
}

FillingFace::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}


void FillingFace::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "FillingFace",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression >> ')' )
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<FillingFace>(*qi::_1, *qi::_2)) ]
    |
    ( '(' >> ruleset.r_edgeFeaturesExpression >> ',' >> ruleset.r_edgeFeaturesExpression >> ')' )
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<FillingFace>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}

bool SingleVolumeFeature::isSingleVolume() const
{
  return true;
}


defineType(Cylinder);
addToFactoryTable(SolidModel, Cylinder, NoParameters);

Cylinder::Cylinder(const NoParameters&)
{}


Cylinder::Cylinder(const arma::mat& p1, const arma::mat& p2, double D)
{
  setShape
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
  );
}

void Cylinder::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cylinder",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	  [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Cylinder>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(Ring);
addToFactoryTable(SolidModel, Ring, NoParameters);

Ring::Ring(const NoParameters&)
{}


Ring::Ring(const arma::mat& p1, const arma::mat& p2, double Da, double Di)
{
  setShape
  (
    BRepAlgoAPI_Cut
    (
      
      BRepPrimAPI_MakeCylinder
      (
	gp_Ax2
	(
	  gp_Pnt(p1(0),p1(1),p1(2)), 
	  gp_Dir(p2(0)-p1(0),p2(1)-p1(1),p2(2)-p1(2))
	),
	0.5*Da, 
	norm(p2-p1, 2)
      ).Shape(),
     
      BRepPrimAPI_MakeCylinder
      (
	gp_Ax2
	(
	  gp_Pnt(p1(0),p1(1),p1(2)), 
	  gp_Dir(p2(0)-p1(0),p2(1)-p1(1),p2(2)-p1(2))
	),
	0.5*Di, 
	norm(p2-p1, 2)
      ).Shape()
      
    ).Shape()   
  );
}

void Ring::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Ring",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	  [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Ring>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}

defineType(Shoulder);
addToFactoryTable(SolidModel, Shoulder, NoParameters);

Shoulder::Shoulder(const NoParameters&)
{}


Shoulder::Shoulder(const arma::mat& p0, const arma::mat& dir, double d, double Dmax)
{
  TopoDS_Face xsec_o=BRepBuilderAPI_MakeFace
  (
    BRepBuilderAPI_MakeWire
    (
      BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2( to_Pnt(p0), to_Vec(dir) ), 0.5*Dmax ))
    )
  );
  TopoDS_Face xsec=BRepBuilderAPI_MakeFace
  (
    xsec_o,
    BRepBuilderAPI_MakeWire
    (
      BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2( to_Pnt(p0), to_Vec(dir) ), 0.5*d ))
    )
  );
  
  setShape
  (
    BRepPrimAPI_MakePrism
    ( 
      xsec, 
//       gp_Dir(to_Vec(dir)), false 
      to_Vec(dir)*1e6
    )
  ); // semi-infinite prism
}

void Shoulder::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Shoulder",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > 
			    ',' > ruleset.r_scalarExpression > ((',' > ruleset.r_scalarExpression)|qi::attr(1e6)) > ')' )
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Shoulder>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}

defineType(Box);
addToFactoryTable(SolidModel, Box, NoParameters);

Box::Box(const NoParameters&)
{}


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
{ 
  setShape(makeBox(p0, L1, L2, L3, centered));
}

void Box::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Box",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression 
		    > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression 
     > ( (  ',' > qi::lit("centered") > qi::attr(true) ) |qi::attr(false) )
     > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Box>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}

defineType(Sphere);
addToFactoryTable(SolidModel, Sphere, NoParameters);

Sphere::Sphere(const NoParameters&)
{}

  
Sphere::Sphere(const arma::mat& p, double D)
{
  setShape
  (
    BRepPrimAPI_MakeSphere
    (
      gp_Pnt(p(0),p(1),p(2)),
      0.5*D
    ).Shape()
  );
}

void Sphere::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Sphere",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Sphere>(qi::_1, qi::_2)) ]
      
    ))
  );
}

defineType(Extrusion);
addToFactoryTable(SolidModel, Extrusion, NoParameters);

Extrusion::Extrusion(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeExtrusion(const SolidModel& sk, const arma::mat& L, bool centered)
{
  if (!centered)
  {
    return BRepPrimAPI_MakePrism( sk, to_Vec(L) ).Shape();
  }
  else
  {
    gp_Trsf trsf;
    trsf.SetTranslation(to_Vec(-0.5*L));
    return BRepPrimAPI_MakePrism
    ( 
      BRepBuilderAPI_Transform(sk, trsf).Shape(), 
      to_Vec(L) 
    ).Shape();
  }
}

Extrusion::Extrusion(const SolidModel& sk, const arma::mat& L, bool centered)
: SolidModel(makeExtrusion(sk, L, centered))
{
}

void Extrusion::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Extrusion",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression
      > ( (  ',' > qi::lit("centered") > qi::attr(true) ) | qi::attr(false) ) 
      > ')' )
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Extrusion>(*qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(Revolution);
addToFactoryTable(SolidModel, Revolution, NoParameters);

Revolution::Revolution(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeRevolution(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double ang, bool centered)
{
  if (!centered)
  {
    return BRepPrimAPI_MakeRevol( sk, gp_Ax1(to_Pnt(p0), gp_Dir(to_Vec(axis))), ang, centered ).Shape();
  }
  else
  {
    gp_Trsf trsf;
    gp_Vec ax=to_Vec(axis);
    ax.Normalize();
    trsf.SetRotation(gp_Ax1(to_Pnt(p0), ax), -0.5*ang);
    return BRepPrimAPI_MakeRevol
    ( 
      BRepBuilderAPI_Transform(sk, trsf).Shape(), 
      gp_Ax1(to_Pnt(p0), gp_Dir(ax)), ang
    ).Shape();
  }
}

Revolution::Revolution(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double ang, bool centered)
: SolidModel(makeRevolution(sk, p0, axis, ang, centered))
{
}

void Revolution::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Revolution",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	  > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression 
       > ( (  ',' > qi::lit("centered") > qi::attr(true) ) | qi::attr(false))
       > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Revolution>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}

defineType(Sweep);
addToFactoryTable(SolidModel, Sweep, NoParameters);

Sweep::Sweep(const NoParameters& nop): SolidModel(nop)
{}


Sweep::Sweep(const std::vector<SolidModelPtr>& secs)
{
  if (secs.size()<2)
    throw insight::Exception("Insufficient number of sections given!");
  
  bool create_solid=false;
  {
    TopoDS_Shape cs0=*secs[0];
    if (cs0.ShapeType()==TopAbs_FACE)
      create_solid=true;
    else if (cs0.ShapeType()==TopAbs_WIRE)
    {
      create_solid=TopoDS::Wire(cs0).Closed();
    }
  }
  
  BRepOffsetAPI_ThruSections sb(create_solid);
 
  BOOST_FOREACH(const SolidModelPtr& skp, secs)
  {
    TopoDS_Wire cursec;
    TopoDS_Shape cs=*skp;
    if (cs.ShapeType()==TopAbs_FACE)
     cursec=BRepTools::OuterWire(TopoDS::Face(cs));
    else if (cs.ShapeType()==TopAbs_WIRE)
    {
     cursec=TopoDS::Wire(cs);
    }
    else if (cs.ShapeType()==TopAbs_EDGE)
    {
     BRepBuilderAPI_MakeWire w;
     w.Add(TopoDS::Edge(cs));
     cursec=w.Wire();
    }
    else
    {
      throw insight::Exception("Incompatible section shape for Sweep!");
    }
    sb.AddWire(cursec);
  }
  
  setShape(sb.Shape());
}

void Sweep::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Sweep",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> (ruleset.r_solidmodel_expression % ',' ) >> ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Sweep>(qi::_1)) ]
      
    ))
  );
}


defineType(Pipe);
addToFactoryTable(SolidModel, Pipe, NoParameters);

Pipe::Pipe(const NoParameters& nop): SolidModel(nop)
{}


Pipe::Pipe(const SolidModel& spine, const SolidModel& xsec)
{
  if (!spine.isSingleWire())
    throw insight::Exception("spine feature has to provide a singly connected wire!");
  
  if (!xsec.isSingleFace() || xsec.isSingleWire() || xsec.isSingleEdge())
    throw insight::Exception("xsec feature has to provide a face or wire!");
  
  TopoDS_Wire spinew=spine.asSingleWire();
  TopoDS_Vertex pfirst, plast;
  TopExp::Vertices( spinew, pfirst, plast );
  
  
  gp_Trsf tr;
  tr.SetTranslation(gp_Vec(BRep_Tool::Pnt(pfirst).XYZ()));
  TopoDS_Shape xsecs=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr).Shape();

//   BRepOffsetAPI_MakePipeShell p(spinew);
//   Handle_Law_Constant law(new Law_Constant());
//   law->Set(1.0, -1e10, 1e10);
//   p.SetLaw(static_cast<TopoDS_Shape>(xsec), law, pfirst);
//   p.SetMode(true);
//   p.MakeSolid();
  
  BRepOffsetAPI_MakePipe p(spinew, xsecs);
  
  p.Build();
  setShape(p.Shape());
}

void Pipe::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Pipe",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression >> ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Pipe>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}


defineType(Bar);
addToFactoryTable(SolidModel, Bar, NoParameters);

Bar::Bar(const NoParameters& nop): SolidModel(nop)
{}


Bar::Bar
(
  const arma::mat& start, const arma::mat& end, 
  const SolidModel& xsec, const arma::mat& vert, 
  double ext0, double ext1,
  double miterangle0_vert, double miterangle1_vert,
  double miterangle0_hor, double miterangle1_hor
)
{
  arma::mat p0=start;
  arma::mat p1=end;
  
  if (norm(vert,2)<1e-10)
    throw insight::Exception("Bar: length of vertical direction is zero!");
  arma::mat v=vert/norm(vert,2);
  
  if (!xsec.isSingleFace() || xsec.isSingleWire() || xsec.isSingleEdge())
    throw insight::Exception("xsec feature has to provide a face or wire!");
  
  arma::mat baraxis=p1-p0;
  double lba=norm(baraxis,2);
  if (lba<1e-10)
    throw insight::Exception("Bar: invalid definition of bar end points!");
  baraxis/=lba;
  
  p0 += -baraxis*ext0;
  p1 +=  baraxis*ext1;
  double L=norm(p1-p0, 2);
  
  TopoDS_Wire spine=BRepBuilderAPI_MakeWire
  (
    BRepBuilderAPI_MakeEdge
    (
      GC_MakeSegment(to_Pnt(p0), to_Pnt(p1))
    )
  );
//   TopoDS_Vertex pfirst, plast;
//   TopExp::Vertices( spine, pfirst, plast );
  
    
  arma::mat ex=-arma::cross(baraxis, vert);
  
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
  TopoDS_Shape xsecs=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr.Inverted()).Shape();

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
  if ( (fabs(miterangle0_vert)>1e-10) || (fabs(miterangle0_hor)>1e-10) )
  {
    arma::mat cex=rotMatrix(miterangle0_vert, ey)*ex;
    arma::mat cey=rotMatrix(miterangle0_hor, ex)*ey;
    Quad q(start-0.5*L*(cex+cey), L*cex, L*cey);
    TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(-L*baraxis) );
    result=BRepAlgoAPI_Cut(result, airspace);
  }
  
  // cut away at end 1
  if ( (fabs(miterangle1_vert)>1e-10) || (fabs(miterangle1_hor)>1e-10) )
  {
    arma::mat cex=rotMatrix(miterangle1_vert, ey)*ex;
    arma::mat cey=rotMatrix(miterangle1_hor, ex)*ey;
    Quad q(end-0.5*L*(cex+cey), L*cex, L*cey);
    TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(L*baraxis) );
    result=BRepAlgoAPI_Cut(result, airspace);
  }
  
  setShape(result);
}

void Bar::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Bar",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
	>> ruleset.r_vectorExpression // 1
	  >> ( ( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(0.0) )
	  >> ( ( qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0) ) 
// 	  >> ( ( qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0) ) 
	  >> ',' 
	>> ruleset.r_vectorExpression // 5
	  >> ( ( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(0.0) ) 
	  >> ( ( qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0) ) 
// 	  >> ( ( qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0) ) 
	  >> ',' 
	>> ruleset.r_solidmodel_expression >> ',' // 9
	>> ruleset.r_vectorExpression >> // 10
      ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Bar>
	(
	  qi::_1, qi::_4, 
	  *qi::_7, qi::_8,
	  qi::_2, qi::_5,
	  qi::_3, qi::_6
// 	  qi::_1, qi::_5, 
// 	  *qi::_9, qi::_10,
// 	  qi::_2, qi::_6,
// 	  qi::_3, qi::_7,
// 	  qi::_4, qi::_8
	)) ]
      
    ))
  );
}

defineType(Thicken);
addToFactoryTable(SolidModel, Thicken, NoParameters);

Thicken::Thicken(const NoParameters& nop): SolidModel(nop)
{}


Thicken::Thicken(const SolidModel& shell, double thickness, double tol)
{

  BRepOffsetAPI_MakeOffsetShape maker(shell, thickness, 0.01,BRepOffset_Skin,Standard_False,Standard_False,GeomAbs_Arc);
  
  setShape(maker.Shape());
}

void Thicken::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Thicken",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_scalarExpression >> ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Thicken>(*qi::_1, qi::_2)) ]
      
    ))
  );
}

defineType(RotatedHelicalSweep);
addToFactoryTable(SolidModel, RotatedHelicalSweep, NoParameters);

RotatedHelicalSweep::RotatedHelicalSweep(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeRotatedHelicalSweep(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double P, double revoffset)
{
//   BRep_Builder bb;
//   TopoDS_Compound result;
//   bb.MakeCompound(result);

  BRepOffsetAPI_ThruSections sb(true);
  
  TopoDS_Wire ow=BRepTools::OuterWire(TopoDS::Face(sk));
  
  double dz=arma::norm(axis, 2);
  arma::mat ez=axis/norm(axis, 2);
  double phi=0.0, dphi=2.*dz*M_PI/P;
  int nstep=std::max( 2, int(ceil(dphi/(M_PI/64.))) );
  double phi_step=dphi/double(nstep);
  
#define TRSF(shape, deltaz, oshape) \
  {\
    gp_Trsf t1, t2;\
    t1.SetTranslation( to_Vec(ez).Scaled(deltaz) );\
    t2.SetRotation( gp_Ax1( to_Pnt(p0), to_Vec(ez) ), 2.*deltaz*M_PI/P );\
    oshape = TopoDS::Wire(BRepBuilderAPI_Transform\
      (\
	BRepBuilderAPI_Transform\
	(\
	  shape, \
	  t1\
	).Shape(),\
        t2\
      ).Shape());\
  }
  
  TopoDS_Wire firstsec;
  TRSF(ow, -revoffset, firstsec);
  
  for (int i=0; i<nstep+1; i++)
  {
    double z=phi*P/(2.*M_PI);
    TopoDS_Wire cursec;
    TRSF(firstsec, z, cursec);
//     bb.Add(result, cursec);
    sb.AddWire(cursec);
    
    phi+=phi_step;
  }
  
//   return result;
  return sb.Shape();
}


RotatedHelicalSweep::RotatedHelicalSweep(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double P, double revoffset)
: SolidModel(makeRotatedHelicalSweep(sk, p0, axis, P, revoffset))
{
}

void RotatedHelicalSweep::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "RotatedHelicalSweep",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
	    > ruleset.r_solidmodel_expression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_scalarExpression > 
	    ((  ',' > ruleset.r_scalarExpression ) | qi::attr(0.0)) > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<RotatedHelicalSweep>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}


defineType(StitchedSolid);
addToFactoryTable(SolidModel, StitchedSolid, NoParameters);

StitchedSolid::StitchedSolid(const NoParameters&)
{}


StitchedSolid::StitchedSolid(const std::vector< SolidModelPtr >& faces, double tol)
{
  BRepBuilderAPI_Sewing sew(tol);
  
//   TopoDS_Compound aRes;
//   BRep_Builder aBuilder;
//   aBuilder.MakeCompound(aRes);

  BOOST_FOREACH(const SolidModelPtr& m, faces)
  {
    sew.Add(*m);
//     aBuilder.Add(aRes, bladeFace_[s]);
  }

  sew.Perform();
  sew.Dump();
  
  TopoDS_Shell sshell = TopoDS::Shell(sew.SewedShape());
  BRepCheck_Shell acheck(sshell);
  
    if (acheck.Closed(Standard_False) != BRepCheck_NoError)
    throw insight::Exception("Could not create a closed shell (B)!");

  if (acheck.Orientation(Standard_False) != BRepCheck_NoError)
    throw insight::Exception("Orientation Error!");
  
  BRepBuilderAPI_MakeSolid solidmaker(sshell);
  
  if (!solidmaker.IsDone())
    throw insight::Exception("Creation of solid failed!");

  setShape(solidmaker.Solid());
}

void StitchedSolid::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "StitchedSolid",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > (ruleset.r_solidmodel_expression % ',') > ( (',' > qi::double_) | qi::attr(1e-3) ) > ')' )
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<StitchedSolid>(qi::_1, qi::_2)) ]
      
    ))
  );
}


defineType(StitchedShell);
addToFactoryTable(SolidModel, StitchedShell, NoParameters);

StitchedShell::StitchedShell(const NoParameters& nop)
: SolidModel(nop)
{
}

StitchedShell::StitchedShell(const FeatureSet& faces, double tol)
: SolidModel()
{
  BRepBuilderAPI_Sewing sew(tol);
  
//   TopoDS_Compound aRes;
//   BRep_Builder aBuilder;
//   aBuilder.MakeCompound(aRes);

  BOOST_FOREACH(const FeatureID& fi, faces)
  {
    sew.Add(faces.model().face(fi));
//     aBuilder.Add(aRes, bladeFace_[s]);
  }

  sew.Perform();
  sew.Dump();
  
  TopoDS_Shell sshell = TopoDS::Shell(sew.SewedShape());
  BRepCheck_Shell acheck(sshell);
  
  
  setShape(sshell);
}

void StitchedShell::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "StitchedShell",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_faceFeaturesExpression  > ( (',' > qi::double_) | qi::attr(1e-3) ) > ')' )
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<StitchedShell>(*qi::_1, qi::_2)) ]
      
    ))
  );
}


defineType(BooleanUnion);
addToFactoryTable(SolidModel, BooleanUnion, NoParameters);

BooleanUnion::BooleanUnion(const NoParameters& nop): SolidModel(nop)
{}

BooleanUnion::BooleanUnion(const SolidModel& m)
{
  m.unsetLeaf();
  
  TopoDS_Shape res;
  for (TopExp_Explorer ex(m, TopAbs_SOLID); ex.More(); ex.Next())
  {
    if (res.IsNull())
      res=TopoDS::Solid(ex.Current());
    else
      res=BRepAlgoAPI_Fuse(res, TopoDS::Solid(ex.Current())).Shape();
  }
  setShape(res);
}

BooleanUnion::BooleanUnion(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Fuse(m1, m2).Shape())
{
  m1.unsetLeaf();
  m2.unsetLeaf();
}

SolidModel operator|(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanUnion(m1, m2);
}

void BooleanUnion::insertrule(parser::ISCADParser& ruleset) const
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


defineType(BooleanSubtract);
addToFactoryTable(SolidModel, BooleanSubtract, NoParameters);

BooleanSubtract::BooleanSubtract(const NoParameters& nop): SolidModel(nop)
{}


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

void BooleanSubtract::insertrule(parser::ISCADParser& ruleset) const
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

defineType(BooleanIntersection);
addToFactoryTable(SolidModel, BooleanIntersection, NoParameters);

BooleanIntersection::BooleanIntersection(const NoParameters& nop): SolidModel(nop)
{}


BooleanIntersection::BooleanIntersection(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Common(m1, m2).Shape())
{
  m1.unsetLeaf();
  m2.unsetLeaf();
}

SolidModel operator&(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanIntersection(m1, m2);
}

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


defineType(Projected);
addToFactoryTable(SolidModel, Projected, NoParameters);

Projected::Projected(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeProjection
(
  const SolidModel& source, 
  const SolidModel& target, 
  const arma::mat& dir
)
{
  TopoDS_Wire ow=BRepTools::OuterWire(TopoDS::Face(source));

  BRepProj_Projection proj(ow, target, to_Vec(dir));
  
  return proj.Shape();
}

Projected::Projected(const SolidModel& source, const SolidModel& target, const arma::mat& dir)
: SolidModel(makeProjection(source, target, dir))
{
}

void Projected::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Projected",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Projected>(*qi::_1, *qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(ProjectedOutline);
addToFactoryTable(SolidModel, ProjectedOutline, NoParameters);

ProjectedOutline::ProjectedOutline(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeOutlineProjection
(
  const SolidModel& source, 
  const Datum& target
)
{
  if (!target.providesPlanarReference())
    throw insight::Exception("Error: Wrong parameter. ProjectedOutline needs a planar reference!");
  
  gp_Ax3 pln=target;
  
  HLRAlgo_Projector projector( pln.Ax2() );
  gp_Trsf transform=projector.FullTransformation();
  
  
  Handle_HLRBRep_Algo brep_hlr = new HLRBRep_Algo;
  brep_hlr->Add( source );
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
//   TopoDS_Shape r1vs=shapes.Rg1LineVCompound();
//   if (!r1vs.IsNull()) builder.Add(allVisible, r1vs);
  TopoDS_Shape olvs = shapes.OutLineVCompound();
  if (!olvs.IsNull()) builder.Add(allVisible, olvs);
  
  return allVisible;
}


TopoDS_Shape makeOutlineProjectionEdges
(
  const SolidModel& source, 
  const Datum& target
)
{
  if (!target.providesPlanarReference())
    throw insight::Exception("Error: Wrong parameter. ProjectedOutline needs a planar reference!");
  
  gp_Ax3 pln=target;
  cout<<"pl"<<endl;
  TopoDS_Face face=BRepBuilderAPI_MakeFace(gp_Pln(pln));
  gp_Trsf trsf;
  trsf.SetTransformation(
    pln,
    gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0))
  );
  
  TopoDS_Compound allVisible;
  BRep_Builder builder;
  builder.MakeCompound( allVisible );

  TopoDS_Shape src=source;
  for (TopExp_Explorer ex(src, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge edge=TopoDS::Edge(ex.Current());
    BRepLib::BuildCurve3d(edge); // just to be sure...

    //     BRepProj_Projection proj(edge, face, pln.Direction().Reversed());
    BRepOffsetAPI_NormalProjection proj(face);
    proj.SetLimit(false);
    proj.Add(edge);
    proj.Build();

    BRepBuilderAPI_Transform tr( proj.Shape(), trsf.Inverted() );

    builder.Add(allVisible, tr.Shape());
  }

  return allVisible;
}

ProjectedOutline::ProjectedOutline(const SolidModel& source, const Datum& target)
//: SolidModel(makeOutlineProjection(source, target))
{
  if (source.allFaces().size()==0)
    setShape(makeOutlineProjectionEdges(source, target));
  else
    setShape(makeOutlineProjection(source, target));
}

void ProjectedOutline::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "ProjectedOutline",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_datumExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<ProjectedOutline>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}

defineType(Split);
addToFactoryTable(SolidModel, Split, NoParameters);

Split::Split(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeSplit(const SolidModel& tool, const SolidModel& target)
{
  GEOMAlgo_Splitter spl;
  spl.AddShape(target);
  spl.AddTool(tool);
  spl.Perform();
  return spl.Shape();
}

Split::Split(const SolidModel& tool, const SolidModel& target)
: SolidModel(makeSplit(tool, target))
{
}

/** @addtogroup cad_parser
  * @{
  * @section split_syntax Split solid
  * Split solid by face
  * 
  * Syntax: Split(<SolidModel>, <SolidModel>)
  * @}
  */
void Split::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Split",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_solidmodel_expression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Split>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}

defineType(Fillet);
addToFactoryTable(SolidModel, Fillet, NoParameters);

Fillet::Fillet(const NoParameters& nop): SolidModel(nop)
{}



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

/** @addtogroup cad_parser
  * @{
  * @section fillet_syntax Fillet syntax
  * Add fillet to edge
  * 
  * Syntax: Fillet(<SolidModel>, <edgeFeatures>, <scalarExpression>)
  * @}
  */
void Fillet::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Fillet",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_edgeFeaturesExpression >> ',' >> ruleset.r_scalarExpression >> ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Fillet>(*qi::_1, *qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(Chamfer);
addToFactoryTable(SolidModel, Chamfer, NoParameters);

Chamfer::Chamfer(const NoParameters& nop): SolidModel(nop)
{}


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

void Chamfer::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Chamfer",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_edgeFeaturesExpression >> ',' >> ruleset.r_scalarExpression >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Chamfer>(*qi::_1, *qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(CircularPattern);
addToFactoryTable(SolidModel, CircularPattern, NoParameters);

CircularPattern::CircularPattern(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape CircularPattern::makePattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n, bool center)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);
  
  double delta_phi=norm(axis, 2);
  double phi0=0.0;
  if (center) phi0=-0.5*delta_phi*double(n-1);
  gp_Ax1 ax(to_Pnt(p0), to_Vec(axis/delta_phi));
  for (int i=0; i<n; i++)
  {
    gp_Trsf tr;
    tr.SetRotation(ax, phi0+delta_phi*double(i));
    bb.Add(result, BRepBuilderAPI_Transform(m1, tr).Shape());
  }
  
  return result;
}
  
CircularPattern::CircularPattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n, bool center)
: SolidModel(makePattern(m1, p0, axis, n, center))
{
  m1.unsetLeaf();
}

void CircularPattern::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "CircularPattern",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	> ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression 
        > ( ( ',' > qi::lit("centered") > qi::attr(true) ) | qi::attr(false) ) 
        > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<CircularPattern>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}


defineType(LinearPattern);
addToFactoryTable(SolidModel, LinearPattern, NoParameters);

LinearPattern::LinearPattern(const NoParameters& nop): SolidModel(nop)
{}


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

void LinearPattern::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "LinearPattern",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > 
      ',' > ruleset.r_vectorExpression > 
      ',' > ruleset.r_scalarExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<LinearPattern>(*qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}


defineType(Transform);
addToFactoryTable(SolidModel, Transform, NoParameters);

Transform::Transform(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape Transform::makeTransform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale)
{
  gp_Trsf tr0, tr1, tr2;
//   TopoDS_Shape intermediate_shape=m1;
//   
  tr0.SetScaleFactor(scale);
//   intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr0).Shape();
// 
  tr1.SetTranslation(to_Vec(trans));  
//   intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr1).Shape();
// 
  double phi=norm(rot, 2);
  if (phi>1e-10)
  {
    gp_Vec axis=to_Vec(rot);
    axis.Normalize();
    tr2.SetRotation(gp_Ax1(gp_Pnt(0,0,0), axis), phi);
//     intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr2).Shape();
  }  
  
  gp_Trsf trcomp=tr2.Multiplied(tr1).Multiplied(tr0);
  return makeTransform(m1, trcomp);
// 
//   // Apply rotation first, then translation
//   return intermediate_shape;
}

TopoDS_Shape Transform::makeTransform(const SolidModel& m1, const gp_Trsf& trsf)
{
  if (m1.hasExplicitCoG())
  {
    this->setCoGExplicitly( vec3(to_Pnt(m1.modelCoG()).Transformed(trsf)) );
  }
  if (m1.hasExplicitMass()) setMassExplicitly(m1.mass());
  return BRepBuilderAPI_Transform(m1, trsf).Shape();
}


Transform::Transform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale)
{
  setShape(makeTransform(m1, trans, rot, scale));
  m1.unsetLeaf();
}

Transform::Transform(const SolidModel& m1, const gp_Trsf& trsf)
{
  setShape(makeTransform(m1, trsf));
  m1.unsetLeaf();
}

void Transform::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Transform",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	> ruleset.r_vectorExpression > ( (',' > ruleset.r_scalarExpression ) | qi::attr(1.0) ) > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Transform>(*qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}


defineType(Mirror);
addToFactoryTable(SolidModel, Mirror, NoParameters);

Mirror::Mirror(const NoParameters& nop): SolidModel(nop)
{}


Mirror::Mirror(const SolidModel& m1, const Datum& pl)
{
  gp_Trsf tr;

  if (!pl.providesPlanarReference())
    throw insight::Exception("Mirror: planar reference required!");
  
  tr.SetMirror(static_cast<gp_Ax3>(pl).Ax2());  
  
  if (m1.hasExplicitCoG())
  {
    this->setCoGExplicitly( vec3(to_Pnt(m1.modelCoG()).Transformed(tr)) );
  }
  if (m1.hasExplicitMass()) setMassExplicitly(m1.mass());
  
  setShape(BRepBuilderAPI_Transform(m1, tr).Shape());
}

void Mirror::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Mirror",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_datumExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Mirror>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}


defineType(Place);
addToFactoryTable(SolidModel, Place, NoParameters);

void Place::makePlacement(const SolidModel& m, const gp_Trsf& tr)
{
  if (m.hasExplicitCoG())
  {
    this->setCoGExplicitly( vec3(to_Pnt(m.modelCoG()).Transformed(tr)) );
  }
  if (m.hasExplicitMass()) setMassExplicitly(m.mass());
  setShape(BRepBuilderAPI_Transform(m, tr).Shape());
}


Place::Place(const NoParameters& nop): SolidModel(nop)
{}


Place::Place(const SolidModel& m, const gp_Ax2& cs)
{
  gp_Trsf tr;
  tr.SetTransformation(gp_Ax3(cs));
  makePlacement(m, tr.Inverted());
}


Place::Place(const SolidModel& m, const arma::mat& p0, const arma::mat& ex, const arma::mat& ez)
{
  gp_Trsf tr;
  tr.SetTransformation(gp_Ax3(to_Pnt(p0), to_Vec(ez), to_Vec(ex)));
  makePlacement(m, tr.Inverted());
}

void Place::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Place",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > 
	  ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Place>(*qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}


defineType(Compound);
addToFactoryTable(SolidModel, Compound, NoParameters);

Compound::Compound(const NoParameters& nop): SolidModel(nop)
{}



Compound::Compound(const std::vector<SolidModelPtr>& m1)
: components_(m1)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);

  BOOST_FOREACH(const SolidModelPtr& p, m1)
  {
    bb.Add(result, *p);
    p->unsetLeaf();
  }
  setShape(result);
}

void Compound::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Compound",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ( ruleset.r_solidmodel_expression % ',' ) > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Compound>(qi::_1)) ]
      
    ))
  );
}

arma::mat Compound::modelCoG() const
{
  if (explicitCoG_.get())
  {
    return *explicitCoG_;
  }
  else
  {
    double mtot=0.0;
    arma::mat cog=vec3(0,0,0);
    
    BOOST_FOREACH(const SolidModelPtr& p, components_)
    {
      const SolidModel& m = *p;
      mtot+=m.mass();
      cog += m.modelCoG()*m.mass();
    }
    
    cout<<"total mass="<<mtot<<endl;
    
    if (mtot<1e-10)
      throw insight::Exception("Total mass is zero!");
    
    cog/=mtot;
    
    cout<<"CoG="<<cog<<endl;
    
    return cog;
  }
}

double Compound::mass() const
{
  if (explicitMass_)
  {
    return *explicitMass_;
  }
  else
  {
    double mtot=0.0;
    
    BOOST_FOREACH(const SolidModelPtr& p, components_)
    {
      const SolidModel& m = *p;
      mtot+=m.mass();
    }
    return mtot;
  }
}




defineType(Cutaway);
addToFactoryTable(SolidModel, Cutaway, NoParameters);

Cutaway::Cutaway(const NoParameters& nop): SolidModel(nop)
{}


Cutaway::Cutaway(const SolidModel& model, const arma::mat& p0, const arma::mat& n)
{
  arma::mat bb=model.modelBndBox(0.1);
  double L=10.*norm(bb.col(1)-bb.col(0), 2);
  std::cout<<"L="<<L<<std::endl;
  
  arma::mat ex=cross(n, vec3(1,0,0));
  if (norm(ex,2)<1e-8)
    ex=cross(n, vec3(0,1,0));
  ex/=norm(ex,2);
  
  arma::mat ey=cross(n,ex);
  ey/=norm(ey,2);
  
  std::cout<<"Quad"<<std::endl;
#warning Relocate p0 in plane to somewhere nearer to model center!
  Quad q(p0-0.5*L*(ex+ey), L*ex, L*ey);
  this->setShape(q);
//   std::cout<<"Airspace"<<std::endl;
  TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(L*n) );
  
//   SolidModel(airspace).saveAs("airspace.stp");
  providedSubshapes_.add("AirSpace", SolidModelPtr(new SolidModel(airspace)));
  
  std::cout<<"CutSurf"<<std::endl;
  try
  {
    providedSubshapes_.add("CutSurface", SolidModelPtr(new BooleanIntersection(model, TopoDS::Face(q))));
  }
  catch (...)
  {
    insight::Warning("Could not create cutting surface!");
  }

  std::cout<<"Cut"<<std::endl;
  try
  {
    this->setShape(BRepAlgoAPI_Cut(model, airspace));
  }
  catch (...)
  {
    throw insight::Exception("Could not create cut!");
  }
}

void Cutaway::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cutaway",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Cutaway>(*qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
