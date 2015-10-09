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

SolidModel::SolidModel(const FeatureSet& feat)
{
  TopoDS_Compound comp;
  BRep_Builder builder;
  builder.MakeCompound( comp );

  BOOST_FOREACH(const FeatureID& id, feat)
  {
    TopoDS_Shape entity;
    if (feat.shape()==Vertex)
    {
      entity=feat.model().vertex(id);
    }
    else if (feat.shape()==Edge)
    {
      entity=feat.model().edge(id);
    }
    else if (feat.shape()==Face)
    {
      entity=feat.model().face(id);
    }
    else if (feat.shape()==Solid)
    {
      entity=feat.model().subsolid(id);
    }
    if (feat.size()==1)
    {
      setShape(entity);
      return;
    }
    else
    {
      builder.Add(comp, entity);
    }
  }
  
  setShape(comp);
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

arma::mat SolidModel::subsolidCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(subsolid(i), props);
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

FeatureSet SolidModel::allSolids() const
{
  FeatureSet f(*this, Solid);
  f.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( somap_.Extent()+1 ) 
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
    bool ok=f->checkMatch(i);
    if (ok) std::cout<<"match! ("<<i<<")"<<std::endl;
    if (ok) res.insert(i);
  }
  cout<<"QUERY_FACES RESULT = "<<res<<endl;
  return res;
}

FeatureSet SolidModel::query_faces_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces_subset(fs, parseFaceFilterExpr(is, refs));
}

FeatureSet SolidModel::query_solids(const FilterPtr& f) const
{
  return query_solids_subset(allSolids(), f);
}

FeatureSet SolidModel::query_solids(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids(parseSolidFilterExpr(is, refs));
}


FeatureSet SolidModel::query_solids_subset(const FeatureSet& fs, const FilterPtr& f) const
{
//   Filter::Ptr f(filter.clone());
  
  f->initialize(*this);
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSet res(*this, Solid);
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_SOLIDS RESULT = "<<res<<endl;
  return res;
}

FeatureSet SolidModel::query_solids_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids_subset(fs, parseSolidFilterExpr(is, refs));
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
  ruleset.modelstepFunctionRules.add
  (
    "asModel",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ( ruleset.r_vertexFeaturesExpression | ruleset.r_edgeFeaturesExpression | ruleset.r_faceFeaturesExpression | ruleset.r_solidFeaturesExpression ) >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<SolidModel>(*qi::_1)) ]
      
    ))
  );
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



bool SingleVolumeFeature::isSingleVolume() const
{
  return true;
}


}
}
