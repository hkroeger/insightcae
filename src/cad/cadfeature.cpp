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

#include "cadfeature.h"
#include "datum.h"
#include "sketch.h"
#include "transform.h"

#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include "boost/make_shared.hpp"

#include "dxfwriter.h"
#include "featurefilter.h"
#include "feature.h"
#include "gp_Cylinder.hxx"

#include <BRepLib_FindSurface.hxx>
#include <BRepCheck_Shell.hxx>
#include "BRepOffsetAPI_NormalProjection.hxx"
#include "HLRBRep_PolyHLRToShape.hxx"
#include "HLRBRep_PolyAlgo.hxx"
#include "GProp_PrincipalProps.hxx"
#include "Standard_Transient.hxx"
// #include "Standard_Transient_proto.hxx"
#include "CDM_Document.hxx"
// #include "MMgt_TShared.hxx"
#include "TDocStd_Document.hxx"
#include "XCAFApp_Application.hxx"
#include "XCAFDoc.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"
#include "XSControl_WorkSession.hxx"
#include "XSControl_TransferReader.hxx"
#include "XSControl_TransferWriter.hxx"
#include "StepData_StepModel.hxx"
#include "TDF_LabelSequence.hxx"
#include "Handle_StepRepr_RepresentationItem.hxx"
#include "STEPConstruct.hxx"
#include "STEPCAFControl_Writer.hxx"
#include "STEPCAFControl_Reader.hxx"
#include "StepRepr_RepresentationItem.hxx"
#include "StepShape_AdvancedFace.hxx"
#include "APIHeaderSection_MakeHeader.hxx"
#include "TDataStd_Name.hxx"
#include "TDF_ChildIterator.hxx"
#include "TDF_ChildIDIterator.hxx"
#include "TransferBRep.hxx"
#include "Transfer_Binder.hxx"
#include "Transfer_TransientProcess.hxx"

#include "openfoam/openfoamdict.h"

#include "TColStd_SequenceOfTransient.hxx"
#include "TColStd_HSequenceOfTransient.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace boost
{
 
std::size_t hash<TopoDS_Shape>::operator()(const TopoDS_Shape& shape) const
{
  insight::cad::Feature m(shape);  
  return m.hash();
}

std::size_t hash<arma::mat>::operator()(const arma::mat& v) const
{
  std::hash<double> dh;
  size_t h=0;
  for (int i=0; i<v.n_elem; i++)
  {
    boost::hash_combine(h, dh(v(i)));
  }
  return h;
}

std::size_t hash<gp_Pnt>::operator()(const gp_Pnt& v) const
{
  std::hash<double> dh;
  size_t h=0;
  boost::hash_combine(h, dh(v.X()));
  boost::hash_combine(h, dh(v.Y()));
  boost::hash_combine(h, dh(v.Z()));
  return h;
}

std::size_t hash<insight::cad::Datum>::operator()(const insight::cad::Datum& m) const
{
  return m.hash();
}

std::size_t hash<insight::cad::Feature>::operator()(const insight::cad::Feature& m) const
{
  return m.hash();
}

std::size_t hash<boost::filesystem::path>::operator()(const boost::filesystem::path& fn) const
{
  size_t h=0;
  // build from file path string and last write time (latter only if file exists)
  boost::hash_combine(h, fn.string());
  if (boost::filesystem::exists(fn))
  {
    boost::hash_combine(h, boost::filesystem::last_write_time(fn));
  }
  return h;
}

std::size_t hash<insight::cad::FeatureSet>::operator()(const insight::cad::FeatureSet& m) const
{
  return m.hash();
}

}


namespace insight 
{
namespace cad 
{


ParameterListHash::ParameterListHash()
: model_(0), hash_(0)
{}

ParameterListHash::ParameterListHash(Feature *m)
: model_(m), hash_(0)
{}


ParameterListHash::operator size_t ()
{
  return hash_;
}




std::ostream& operator<<(std::ostream& os, const Feature& m)
{
  os<<"ENTITIES\n================\n\n";
  BRepTools::Dump(m.shape(), os);
  os<<"\n================\n\n";
  return os;
}


FeatureCmdInfo::FeatureCmdInfo
(
    std::string command,
    std::string signature,
    std::string documentation
)
: command_(command), 
  signature_(signature), 
  documentation_(documentation)
{}


defineType(Feature);
defineFactoryTableNoArgs(Feature);
addToFactoryTable(Feature, Feature);


void Feature::loadShapeFromFile(const boost::filesystem::path& filename)
{
    cout<<"Reading "<<filename<<endl;

    std::string ext=filename.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext==".brep")
    {
        BRep_Builder bb;
        TopoDS_Shape s;
        BRepTools::Read(s, filename.c_str(), bb);
        setShape(s);
    }
    else if ( (ext==".igs") || (ext==".iges") )
    {
        IGESControl_Reader igesReader;

        igesReader = IGESControl_Reader();
        igesReader.ReadFile(filename.c_str());
        igesReader.TransferRoots();

        setShape(igesReader.OneShape());
    }
    else if ( (ext==".stp") || (ext==".step") )
    {
        STEPControl_Reader reader;
        reader = STEPControl_Reader();
        reader.ReadFile(filename.c_str());
        reader.TransferRoots();
        TopoDS_Shape res=reader.OneShape();
        
        setShape(res);

        typedef std::map<std::string, FeatureSetPtr> Feats;
        Feats feats;

        //Handle_TColStd_HSequenceOfTransient shapeList = reader.GiveList("xst-model-roots");
        Handle_TColStd_HSequenceOfTransient shapeList = reader.GiveList("xst-model-all");
        reader.TransferList(shapeList);


        for(int i=1; i <= shapeList->Length(); i++)
        {
            Handle_Standard_Transient transient = shapeList->Value(i);
            const Handle_XSControl_WorkSession & theSession = reader.WS();
            const Handle_XSControl_TransferReader & aReader = theSession->TransferReader();
            const Handle_Transfer_TransientProcess & tp = aReader->TransientProcess();
            TopoDS_Shape shape = TransferBRep::ShapeResult(tp, transient);
            if(!shape.IsNull())
            {
                Handle_Standard_Transient anEntity = aReader->EntityFromShapeResult(shape, 1);
                if(!anEntity.IsNull())
                {
                    Handle_StepRepr_RepresentationItem entity = Handle_StepRepr_RepresentationItem::DownCast(anEntity);

                    if(!entity.IsNull())
                    {
                        if (!entity->Name()->IsEmpty())
                        {
                            std::string n(entity->Name()->ToCString());
                            std::cout<<"\""<<n<<"\""<<std::endl;
                            if (shape.ShapeType()==TopAbs_FACE)
                            {
                                int found=0;
                                FeatureID id=-1;
                                for(TopExp_Explorer ex(res, TopAbs_FACE); ex.More(); ex.Next())
                                {
                                    TopoDS_Face f=TopoDS::Face(ex.Current());
                                    if (f.IsPartner(shape))
                                    {
                                        found++;
                                        id=faceID(f);
                                        std::cout<<"MATCH! face id="<<id<<std::endl;

                                    }
                                }
                                if (!found)
                                {
                                    throw insight::Exception("could not identify named face in model! ("+n+")");
                                }
                                else if (found>1)
                                {
                                    throw insight::Exception(boost::str(boost::format("identified too many named faces in model! (%s found %d times)") % n % found));
                                }
                                else
                                {
                                    std::string name="face_"+n;
                                    if (feats.find(name)==feats.end())
                                        feats[name].reset(new FeatureSet(shared_from_this(), Face));
                                    feats[name]->add(id);
                                }
                            }
                            else if (shape.ShapeType()==TopAbs_SOLID)
                            {
                                int found=0;
                                FeatureID id=-1;
                                for(TopExp_Explorer ex(res, TopAbs_SOLID); ex.More(); ex.Next())
                                {
                                    TopoDS_Solid f=TopoDS::Solid(ex.Current());
                                    if (f.IsPartner(shape))
                                    {
                                        found++;
                                        id=solidID(f);
                                        std::cout<<"MATCH! solid id="<<id<<std::endl;

                                    }
                                }
                                if (!found)
                                {
                                    throw insight::Exception("could not identify named solid in model! ("+n+")");
                                }
                                else if (found>1)
                                {
                                    throw insight::Exception(boost::str(boost::format("identified too many named solids in model! (%s found %d times)") % n % found));
                                }
                                else
                                {
                                    std::string name="solid_"+n;
                                    if (feats.find(name)==feats.end())
                                        feats[name].reset(new FeatureSet(shared_from_this(), Solid));
                                    feats[name]->add(id);
                                }
                            }
                        }
                    }
                }
            }
        }


        BOOST_FOREACH(Feats::value_type& f, feats)
        {
            providedFeatureSets_[f.first]=f.second;
            providedSubshapes_[f.first].reset(new Feature(f.second));
        }

    }
    else
    {
        throw insight::Exception("Unknown import file format! (Extension "+ext+")");
//     return TopoDS_Shape();
    }
}

void Feature::setShapeHash()
{
  // create hash from
  // 1. total volume
  // 2. # vertices
  // 3. # faces
  // 4. vertex locations
  
  boost::hash_combine(hash_, boost::hash<double>()(modelVolume()));
  boost::hash_combine(hash_, boost::hash<int>()(vmap_.Extent()));
  boost::hash_combine(hash_, boost::hash<int>()(fmap_.Extent()));

  FeatureSetData vset=allVerticesSet();
  BOOST_FOREACH(const insight::cad::FeatureID& j, vset)
  {
    boost::hash_combine
    (
      hash_, 
      boost::hash<arma::mat>()(vertexLocation(j))
    );    
  }
}

void Feature::calcHash()
{
  hash_=0.0;
  if (valid()) setShapeHash();
}


void Feature::updateVolProps() const
{
  if (!volprops_)
  {
    volprops_.reset(new GProp_GProps());
    BRepGProp::VolumeProperties(shape(), *volprops_);
  }
}

void Feature::setShape(const TopoDS_Shape& shape)
{
  volprops_.reset();
  shape_=shape;
  nameFeatures();
  setValid();
}


Feature::Feature()
: isleaf_(true),
//   density_(1.0),
//   areaWeight_(0.0),
  hash_(0)
{
}

Feature::Feature(const Feature& o)
: isleaf_(true),
  providedSubshapes_(o.providedSubshapes_),
  providedFeatureSets_(o.providedFeatureSets_),
  providedDatums_(o.providedDatums_),
  density_(o.density_),
  areaWeight_(o.areaWeight_),
  hash_(o.hash_)
{
  setShape(o.shape_);
}

Feature::Feature(const TopoDS_Shape& shape)
: isleaf_(true),
//   density_(1.0),
//   areaWeight_(0.0),
  hash_(0)
{
  setShape(shape);
  setShapeHash();
  setValid();
}

// Feature::Feature(const boost::filesystem::path& filepath)
// : isleaf_(true),
// //   density_(1.0),
// //   areaWeight_(0.0),
//   hash_(0)
// {
//   loadShapeFromFile(filepath);
//   setShapeHash();
//   setValid();
// }

Feature::Feature(FeatureSetPtr creashapes)
: creashapes_(creashapes)
{}

FeaturePtr Feature::CreateFromFile(const boost::filesystem::path& filepath)
{
  FeaturePtr f(new Feature());
  f->loadShapeFromFile(filepath);
  f->setShapeHash();
  f->setValid();
  return f;
}

Feature::~Feature()
{
}

void Feature::setDensity(ScalarPtr rho) 
{ 
  density_=rho; 
}
  

double Feature::density() const 
{ 
  if (density_)
    return density_->value(); 
  else
    return 1.0;
}

void Feature::setAreaWeight(ScalarPtr rho) 
{ 
  areaWeight_=rho; 
}

double Feature::areaWeight() const 
{ 
  if (areaWeight_)
    return areaWeight_->value(); 
  else
    return 0.0;
}

double Feature::mass(double density_ovr, double aw_ovr) const
{
  checkForBuildDuringAccess();
  double rho=density();
  if (density_ovr>=0.) rho=density_ovr;
  
  double aw=areaWeight();
  if (aw_ovr>=0.) aw=aw_ovr;
  
//   std::cout<<rho<<" ("<<density_ovr<<")"<<std::endl;
  
  double mtot=rho*modelVolume() + aw*modelSurfaceArea();
//   cout<<"Computed mass rho / V = "<<rho<<" / "<<modelVolume()
//       <<", mf / A = "<<aw<<" / "<<modelSurfaceArea()
//       <<", m = "<<mtot<<endl;
  return mtot;
}

void Feature::build()
{
  TopoDS_Compound comp;
  BRep_Builder builder;
  builder.MakeCompound( comp );

  BOOST_FOREACH(const FeatureID& id, creashapes_->data())
  {
    TopoDS_Shape entity;
    if (creashapes_->shape()==Vertex)
    {
      entity=creashapes_->model()->vertex(id);
    }
    else if (creashapes_->shape()==Edge)
    {
      entity=creashapes_->model()->edge(id);
    }
    else if (creashapes_->shape()==Face)
    {
      entity=creashapes_->model()->face(id);
    }
    else if (creashapes_->shape()==Solid)
    {
      entity=creashapes_->model()->subsolid(id);
    }
    if (creashapes_->size()==1)
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


FeaturePtr Feature::subshape(const std::string& name)
{
  checkForBuildDuringAccess();
  SubfeatureMap::iterator i = providedSubshapes_.find(name);
  if (i==providedSubshapes_.end())
  {
    throw insight::Exception("Subfeature "+name+" is not present!");
    return FeaturePtr();
  }
  else
  {
    return i->second;
  }
}


FeatureSetPtr Feature::providedFeatureSet(const std::string& name)
{
  checkForBuildDuringAccess();
  FeatureSetPtrMap::iterator i = providedFeatureSets_.find(name);
  if (i==providedFeatureSets_.end())
  {
    throw insight::Exception("Feature set "+name+" is not present!");
    return FeatureSetPtr();
  }
  else
  {
    return i->second;
  }
}




Feature& Feature::operator=(const Feature& o)
{
  ASTBase::operator=(o);
  isleaf_=o.isleaf_;
  
  providedSubshapes_=o.providedSubshapes_;
  providedFeatureSets_=o.providedFeatureSets_;
  providedDatums_=o.providedDatums_;
  refvalues_=o.refvalues_;
  refpoints_=o.refpoints_;
  refvectors_=o.refvectors_;
  
  visresolution_=o.visresolution_;
  density_=o.density_;
  areaWeight_=o.areaWeight_;
  hash_=o.hash_;

  if (o.valid())
  {
    setShape(o.shape_);
  }
  return *this;
}

bool Feature::operator==(const Feature& o) const
{
  return shape()==o.shape();
}


GeomAbs_CurveType Feature::edgeType(FeatureID i) const
{
  const TopoDS_Edge& e = edge(i);
  double t0, t1;
  Handle_Geom_Curve crv=BRep_Tool::Curve(e, t0, t1);
  GeomAdaptor_Curve adapt(crv);
  return adapt.GetType();
}

GeomAbs_SurfaceType Feature::faceType(FeatureID i) const
{
  const TopoDS_Face& f = face(i);
  double t0, t1;
  Handle_Geom_Surface surf=BRep_Tool::Surface(f);
  GeomAdaptor_Surface adapt(surf);
  return adapt.GetType();
}

arma::mat Feature::vertexLocation(FeatureID i) const
{
  gp_Pnt cog=BRep_Tool::Pnt(vertex(i));
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::edgeCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::LinearProperties(edge(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::faceCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::SurfaceProperties(face(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::subsolidCoG(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(subsolid(i), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

double Feature::subsolidVolume(FeatureID i) const
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(subsolid(i), props);
  double m = props.Mass();
  return m;
}


arma::mat Feature::modelCoG(double density_ovr) const
{
  checkForBuildDuringAccess();
//   GProp_GProps props;
//   TopoDS_Shape sh=shape();
//   BRepGProp::VolumeProperties(sh, props);
  updateVolProps();
  gp_Pnt cog = volprops_->CentreOfMass();
  return vec3(cog);
}

arma::mat Feature::surfaceCoG(double areaWeight_ovr) const
{
  GProp_GProps props;
  BRepGProp::SurfaceProperties(shape(), props);
  gp_Pnt cog = props.CentreOfMass();
  return insight::vec3( cog.X(), cog.Y(), cog.Z() );
}

arma::mat Feature::surfaceInertia(int axis) const
{
  GProp_GProps props;
  BRepGProp::SurfaceProperties(shape(), props);
  GProp_PrincipalProps pcp = props.PrincipalProperties();
  double ix, iy, iz;
  pcp.Moments(ix, iy, iz);
  gp_Vec ax;
  if (axis==0)
  {
      ax=pcp.FirstAxisOfInertia().Normalized()*std::max(1e-9, ix);
  }
  else if (axis==1)
  {
      ax=pcp.SecondAxisOfInertia().Normalized()*std::max(1e-9, iy);
  }
  else if (axis==2)
  {
      ax=pcp.ThirdAxisOfInertia().Normalized()*std::max(1e-9, iz);
  }
  else
  {
      throw insight::Exception(boost::str(boost::format("surfaceInertia: improper selection of axis (%d)!")%axis));
  }
  return insight::vec3( ax.X(), ax.Y(), ax.Z() );
}

double Feature::modelVolume() const
{
  updateVolProps();
  return volprops_->Mass();
//   TopoDS_Shape sh=shape();
//   TopExp_Explorer ex(sh, TopAbs_SOLID);
//   if (ex.More())
//   {
//     GProp_GProps props;
//     BRepGProp::VolumeProperties(sh, props);
//     return props.Mass();
//   }
//   else
//   {
//     return 0.;
//   }
}

double Feature::modelSurfaceArea() const
{
  checkForBuildDuringAccess();
  TopoDS_Shape sh=shape();
  TopExp_Explorer ex(sh, TopAbs_FACE);
  if (ex.More())
  {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(sh, props);
    return props.Mass();
  }
  else
  {
    return 0.;
  }
}

double Feature::minDist(const arma::mat& p) const
{
  BRepExtrema_DistShapeShape dss
  (
    BRepBuilderAPI_MakeVertex(to_Pnt(p)).Vertex(), 
    shape()
  );
  
  if (!dss.Perform())
    throw insight::Exception("determination of minimum distance to point failed!");
  return dss.Value();
}

double Feature::maxVertexDist(const arma::mat& p) const
{
  double maxdist=0.;
  for (TopExp_Explorer ex(shape(), TopAbs_VERTEX); ex.More(); ex.Next())
  {
    TopoDS_Vertex v=TopoDS::Vertex(ex.Current());
    arma::mat vp=insight::Vector(BRep_Tool::Pnt(v)).t();
    maxdist=std::max(maxdist, norm(p-vp,2));
  }
  return maxdist;
}

double Feature::maxDist(const arma::mat& p) const
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
  return ::sqrt(maxdistsq);
}


arma::mat Feature::modelInertia(double density_ovr) const
{
  checkForBuildDuringAccess();

  updateVolProps();

  double rho=density();
  if (density_ovr>=0.) rho=density_ovr;

  GProp_PrincipalProps gpp = volprops_->PrincipalProperties();
  arma::mat T=arma::zeros(3,3);
  T.col(0)=vec3(gpp.FirstAxisOfInertia());
  T.col(1)=vec3(gpp.SecondAxisOfInertia());
  T.col(2)=vec3(gpp.ThirdAxisOfInertia());
  arma::mat Ip=arma::zeros(3,3);
  gpp.Moments( Ip(0,0), Ip(1,1), Ip(2,2) );
  
  return rho*T*Ip*T.t();
}

arma::mat Feature::modelBndBox(double deflection) const
{
  checkForBuildDuringAccess();
  
  if (deflection>0)
  {
      BRepMesh_IncrementalMesh Inc(shape(), deflection);
  }

  Bnd_Box boundingBox;
  BRepBndLib::Add(shape(), boundingBox);

  arma::mat x=arma::zeros(3,2);
  double g=boundingBox.GetGap();
//   cout<<"gap="<<g<<endl;
  boundingBox.Get
  (
    x(0,0), x(1,0), x(2,0), 
    x(0,1), x(1,1), x(2,1)
  );
  x.col(0)+=g;
  x.col(1)-=g;

  return x;
}


arma::mat Feature::faceNormal(FeatureID i) const
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

FeatureSetData Feature::allVerticesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( vmap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSetData Feature::allEdgesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( emap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSetData Feature::allFacesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( fmap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSetData Feature::allSolidsSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;  
  fsd.insert(
    boost::counting_iterator<int>( 1 ), 
    boost::counting_iterator<int>( somap_.Extent()+1 ) 
  );
  return fsd;
}

FeatureSet Feature::allVertices() const
{
  FeatureSet f(shared_from_this(), Vertex);
  f.setData(allVerticesSet());
  return f;
}

FeatureSet Feature::allEdges() const
{
  FeatureSet f(shared_from_this(), Edge);
  f.setData(allEdgesSet());
  return f;
}

FeatureSet Feature::allFaces() const
{
  FeatureSet f(shared_from_this(), Edge);
  f.setData(allFacesSet());
  return f;
}

FeatureSet Feature::allSolids() const
{
  FeatureSet f(shared_from_this(), Solid);
  f.setData(allSolidsSet());
  return f;
}

FeatureSetData Feature::query_vertices(FilterPtr f) const
{
  return query_vertices_subset(allVertices(), f);
}

FeatureSetData Feature::query_vertices(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_vertices(parseVertexFilterExpr(is, refs));
}


FeatureSetData Feature::query_vertices_subset(const FeatureSetData& fs, FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_VERTICES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_vertices_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_vertices_subset(fs, parseVertexFilterExpr(is, refs));
}



FeatureSetData Feature::query_edges(FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  return query_edges_subset(allEdges(), f);
}

FeatureSetData Feature::query_edges(const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_edges(parseEdgeFilterExpr(is, refs));
}

FeatureSetData Feature::query_edges_subset(const FeatureSetData& fs, FilterPtr f) const
{
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  //for (int i=1; i<=emap_.Extent(); i++)
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  //for (int i=1; i<=emap_.Extent(); i++)
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_EDGES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_edges_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_edges_subset(fs, parseEdgeFilterExpr(is, refs));
}

FeatureSetData Feature::query_faces(FilterPtr f) const
{
  return query_faces_subset(allFaces(), f);
}

FeatureSetData Feature::query_faces(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces(parseFaceFilterExpr(is, refs));
}


FeatureSetData Feature::query_faces_subset(const FeatureSetData& fs, FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  BOOST_FOREACH(int i, fs)
  {
    bool ok=f->checkMatch(i);
    if (ok) std::cout<<"match! ("<<i<<")"<<std::endl;
    if (ok) res.insert(i);
  }
  cout<<"QUERY_FACES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_faces_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces_subset(fs, parseFaceFilterExpr(is, refs));
}

FeatureSetData Feature::query_solids(FilterPtr f) const
{
  return query_solids_subset(allSolids(), f);
}

FeatureSetData Feature::query_solids(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids(parseSolidFilterExpr(is, refs));
}


FeatureSetData Feature::query_solids_subset(const FeatureSetData& fs, FilterPtr f) const
{
//   Filter::Ptr f(filter.clone());
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  BOOST_FOREACH(int i, fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  BOOST_FOREACH(int i, fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  cout<<"QUERY_SOLIDS RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_solids_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids_subset(fs, parseSolidFilterExpr(is, refs));
}

FeatureSet Feature::verticesOfEdge(const FeatureID& e) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  fsd.insert(vmap_.FindIndex(TopExp::FirstVertex(edge(e))));
  fsd.insert(vmap_.FindIndex(TopExp::LastVertex(edge(e))));
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfEdges(const FeatureSet& es) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  BOOST_FOREACH(FeatureID i, es.data())
  {
    FeatureSet j=verticesOfEdge(i);
    fsd.insert(j.data().begin(), j.data().end());
  }
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfFace(const FeatureID& f) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  for (TopExp_Explorer ex(face(f), TopAbs_VERTEX); ex.More(); ex.Next())
  {
    fsd.insert(vmap_.FindIndex(TopoDS::Vertex(ex.Current())));
  }
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfFaces(const FeatureSet& fs) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  BOOST_FOREACH(FeatureID i, fs.data())
  {
    FeatureSet j=verticesOfFace(i);
    fsd.insert(j.data().begin(), j.data().end());
  }
  vertices.setData(fsd);
  return vertices;
}

// Function "StepShape_SetName" :
// Parameters :
// Template type : The type corresponding to the down-cast of the writer representation e.g. 
//     For a solid : StepShape_ManifoldSolidBrep, a face : StepShape_AdvancedFace, etc.
// Handle(StepRepr_RepresentationItem) r = the STEP Writter representation entity obtained by 
//      STEPConstruct::FindEntity(writer.Writer().WS()->TransferWriter()->FinderProcess(), shape)
// STEPCAFControl_Reader & reader = the STEP Reader of the input file
// TopoDS_Shape & shape = the shape that has to be named either from the defined name in the Reader, or with a string "prefix+i"
// char * prefix = name's prefix to set a new name (if the shapes's name undefined in the reader)
// int i = ID to append to the prefix (used to obtain unique names)
// Return values :
//   0 : failure (r is nul or not convertible into Handle_StepShape_TypeX
//   1 : the shape's name is already defined in the input file, and has been copied to the writer
//   2 : the shape's name is undefined in the input file, and has been initialised with "prefix"+i in the writer
// template <class Handle_StepShape_TypeX>
// int StepShape_SetName (Handle_StepRepr_RepresentationItem r, 
//                               STEPCAFControl_Reader & reader, TopoDS_Shape & shape, char * prefix, int i)
// {
//    if (r.IsNull() == Standard_False)
//    {
//       // cast the StepRepr_RepresentationItem to a StepShape_AdvancedFace of the STEP Writer : variable "x"
//       Handle_StepShape_TypeX x = Handle_StepShape_TypeX::DownCast(r);
//       if (r.IsNull() == Standard_True)
//       {
//          std::cerr << "Failed to Down-cast StepRepr_RepresentationItem into "<< x->DynamicType()->Name()<<std::endl;
//          return 0;
//       }
//       char readerName[512];
//       // map the TopoDS_Face to the name of the OCAF STEP Reader (variable "readerName"), 
//       // (the TopoDS_Face is mapped to a STEP Reader StepRepr_RepresentationItem 
//       // inside the function STEP_GetEntityName(), and its name is retrieved )
//       STEP_GetEntityName(shape, &reader, readerName);            
//       if (strlen(readerName) > 0  && strcmp("NONE", readerName) != 0 )
//       {
//          // Associate the STEP Reader face's name to the STEP Writer face name
//          Handle(TCollection_HAsciiString) newid = new TCollection_HAsciiString(readerName);
//          x->SetName(newid);
//          std::cout << "The read Entity is already named : input name \""<< readerName<<"\" is copied to the written output "<<std::endl;
//          return 1;
//       }
//       else
//       {
//          // Associate a non-empty name to the entity
//          char newName[512] = "";
//          sprintf(newName, "%s#%d", prefix, i);
//          Handle(TCollection_HAsciiString) newid = new TCollection_HAsciiString(newName);
//          x->SetName(newid);   
//          std::cout << "The name of the read Entity is undefined : the name is set to \""<< newName<<"\" in the written output"<<std::endl;
//          return 2;
//       }
//    }
//    return 0;
// }

void Feature::saveAs
(
  const boost::filesystem::path& filename, 
  const std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> >& namedfeats
) const
{
  checkForBuildDuringAccess();
  

  std::string ext=filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  cout<<filename<<" >> "<<ext<<endl;
  if (ext==".brep")
  {
    BRepTools::Write(shape(), filename.c_str());
  } 
  else if ( (ext==".igs") || (ext==".iges") )
  {
    Interface_Static::SetIVal("write.iges.brep.mode", 1);
    IGESControl_Controller igesctrl;
    igesctrl.Init();
    IGESControl_Writer igeswriter;
    igeswriter.AddShape(shape());
    igeswriter.Write(filename.c_str());
  } 
  else if ( (ext==".stp") || (ext==".step") )
  {
//     STEPControl_Writer stepwriter;
//     stepwriter.Transfer(shape(), STEPControl_AsIs);
//     stepwriter.Write(filename.c_str());
    
    



   IFSelect_ReturnStatus stat;
   
   // The various ways of reading a file are available here too :
   // to read it by the reader, to take it from a WorkSession ...
   Handle_TDocStd_Document aDoc;
   {
      Handle_XCAFApp_Application anApp = XCAFApp_Application::GetApplication();
      anApp->NewDocument("MDTV-XCAF", aDoc);
   }
//    if ( !reader.Transfer ( aDoc ) ) {
//       std::cout<<"Cannot read any relevant data from the STEP file"<<endl;
//       // abandon ..
//    }

   Standard_Boolean ok; // = reader.Transfer(aDoc);
   Handle_XCAFDoc_ShapeTool myAssembly = XCAFDoc_DocumentTool::ShapeTool(aDoc->Main());   
   myAssembly->AddShape(shape());
   
   STEPControl_StepModelType mode = STEPControl_AsIs;
   STEPCAFControl_Writer writer;

   Interface_Static::SetCVal("write.step.schema", "AP214IS");
   Interface_Static::SetCVal("write.step.product.name", "productInfo" );

   // configure STEP interface
   writer.SetColorMode(Standard_True);
   writer.SetLayerMode(Standard_True);
   writer.SetNameMode (Standard_True);

   // Translating document (conversion) to STEP Writer
   if ( !writer.Transfer ( aDoc, mode ) ) {
      throw insight::Exception("The document cannot be translated or gives no result");
//       return Standard_False;
   }

//    // This is just to see the difference of STEP Files with and without names :
//    // Write the File (WITHOUT names associated to Faces/Edges/Vertices) 
//    ok=Standard_True;
//    char outfilename2[256]="";
//    sprintf(outfilename2, "%s_out_MISSING_NAMES.step", filename.stem().c_str());
//    stat = writer.Write(outfilename2);
//    if (stat!=IFSelect_RetDone) ok = Standard_False;

//    // List interesting stuff : class names of the StepRepr_Items in the STEP Writer :-)
//    // These class names have to be used to map TopoDS_Entities to classes of the STEP Writer
//    std::map <std::string, int> TransientCounter;
//    Handle_StepData_StepModel stpDataModel = writer.Writer().Model();
//    for (int i=1; i<=stpDataModel->NbEntities(); i++)
//    {
// 
//       Handle_Standard_Transient e = stpDataModel->Entity(i);
// 
//       if (TransientCounter.find((e->DynamicType())->Name()) == TransientCounter.end())
//          TransientCounter[(e->DynamicType())->Name()] = 1;
//       else
//          TransientCounter[(e->DynamicType())->Name()] += 1;
//    }
//    for (std::map <std::string, int>::iterator it=TransientCounter.begin(); it != TransientCounter.end(); it++)
//    {
//       std::cout<<it->second<<" STEP Writer entities of type\""<<it->first << "\""<<std::endl;
//    }

   ////// Begin Hack to associate names to Faces/Edges/Vertices /////
   const Handle_XSControl_WorkSession& theSession = writer.Writer().WS();
   const Handle_XSControl_TransferWriter& aTransferWriter =
      theSession->TransferWriter();
   const Handle_Transfer_FinderProcess FP = aTransferWriter->FinderProcess();
   
   typedef std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> > FSM;
   BOOST_FOREACH(const FSM::value_type& fp, namedfeats)
   {
     std::string name = boost::fusion::get<0>(fp); // fp.first;
     const FeatureSetPtr& fs = boost::fusion::get<1>(fp); //fp.second;
     
     if ( fs->shape() == Face )
     {
       BOOST_FOREACH(const FeatureID& id, fs->data())
       {
	 TopoDS_Face aFace = fs->model()->face(id);
	 Handle_StepRepr_RepresentationItem r = STEPConstruct::FindEntity(FP, aFace);
	 if (r.IsNull() == Standard_False)
	 {
	    // cast the StepRepr_RepresentationItem to a StepShape_AdvancedFace of the STEP Writer : variable "x"
	    Handle_StepShape_AdvancedFace x = Handle_StepShape_AdvancedFace::DownCast(r);
	    if (x.IsNull() == Standard_True)
	    {
	      throw insight::Exception
	      (
		"Failed to Down-cast StepRepr_RepresentationItem into "+ std::string(x->DynamicType()->Name())
	      );
	    }
	    
	    Handle(TCollection_HAsciiString) newid = new TCollection_HAsciiString(name.c_str());
	    x->SetName(newid);
#warning check, if already named?
	    
	 } else
	 {
	    throw insight::Exception
	    (
	      "Feature set item not found in model"
	    );
	 }
// 	 StepShape_SetName<Handle_StepShape_AdvancedFace> (r, reader, aFace, "My Face", i++);
       }
     } else
     {
       throw insight::Exception("Given feature set not consisting of faces: yet unsupported");
     }
   }
   
/*
   // Browse "Products" in the sequence of objects in the assembly document
   TDF_LabelSequence labelSequence;
   myAssembly->GetShapes(labelSequence);
   for (int iLabelSeq=1; iLabelSeq<=labelSequence.Length(); iLabelSeq++)
   {
      TopoDS_Shape result   = myAssembly->GetShape(labelSequence.Value(iLabelSeq));
      TopoDS_Solid aSolid; aSolid.Nullify();

      try {
         aSolid=TopoDS::Solid(result);
      }
      catch(Standard_Failure)
      {
      }
      if (!aSolid.IsNull())
      {
         int i=0;
         {
            // map the TopoDS_Solid to a StepRepr_RepresentationItem of the STEP Writer : variable "r"
            Handle_StepRepr_RepresentationItem r = STEPConstruct::FindEntity(FP, aSolid);
            StepShape_SetName<Handle_StepShape_ManifoldSolidBrep> (r, reader, aSolid, "My Solid", i++);
         }

         for(TopExp_Explorer faceExplorer(aSolid, TopAbs_FACE); faceExplorer.More();
            faceExplorer.Next())
         {
            TopoDS_Face aFace = TopoDS::Face(faceExplorer.Current());
            Handle_StepRepr_RepresentationItem r = STEPConstruct::FindEntity(FP, aFace);
            StepShape_SetName<Handle_StepShape_AdvancedFace> (r, reader, aFace, "My Face", i++);
         }

         for(TopExp_Explorer edgeExplorer(aSolid, TopAbs_EDGE); edgeExplorer.More();edgeExplorer.Next()){
            TopoDS_Edge anEdge = TopoDS::Edge(edgeExplorer.Current());
            Handle_StepRepr_RepresentationItem r = STEPConstruct::FindEntity(FP, anEdge);
            StepShape_SetName<Handle_StepShape_EdgeCurve> (r, reader, anEdge, "My Edge", i++);
         }

         for(TopExp_Explorer vertexExplorer(aSolid, TopAbs_VERTEX); vertexExplorer.More();vertexExplorer.Next()){
            TopoDS_Vertex aVertex = TopoDS::Vertex(vertexExplorer.Current());
            Handle_StepRepr_RepresentationItem r = STEPConstruct::FindEntity(FP, aVertex);
            StepShape_SetName<Handle_StepShape_VertexPoint> (r, reader, aVertex, "My Vertex", i++);
         }
      }
   }
   ////// END of the Hack to associate names to Faces/Edges/Vertices /////
*/
   // edit STEP header
   APIHeaderSection_MakeHeader makeHeader(writer.Writer().Model());

//    char outfilename[256]="";
//    sprintf(outfilename, "%s_out.step", filename.stem().string());
   Handle_TCollection_HAsciiString headerFileName = new TCollection_HAsciiString(filename.stem().c_str());
//    Handle(TCollection_HAsciiString) headerAuthor      = new TCollection_HAsciiString("silentdynamics GmbH");
   Handle_TCollection_HAsciiString headerOrganization = new TCollection_HAsciiString("silentdynamics GmbH");
   Handle_TCollection_HAsciiString headerOriginatingSystem = new TCollection_HAsciiString("Insight CAD");
   Handle_TCollection_HAsciiString fileDescription = new TCollection_HAsciiString("iscad model");

   makeHeader.SetName(headerFileName);
//    makeHeader.SetAuthorValue (1, headerAuthor);
   makeHeader.SetOrganizationValue (1, headerOrganization);
   makeHeader.SetOriginatingSystem(headerOriginatingSystem);
   makeHeader.SetDescriptionValue(1, fileDescription);

   // Writing the File (With names associated to Faces/Edges/Vertices)
   ok=Standard_True;
   stat = writer.Write(filename.string().c_str());
   if (stat!=IFSelect_RetDone) ok = Standard_False;
   
   if (!ok) throw insight::Exception("STEP export failed!");
    
  } 
  else if ( (ext==".stl") || (ext==".stlb") )
  {
    BRepMesh_IncrementalMesh Inc(shape(), 1e-2);
    StlAPI_Writer stlwriter;

    stlwriter.ASCIIMode() = (ext==".stl");
    //stlwriter.RelativeMode()=false;
    //stlwriter.SetDeflection(maxdefl);
#if OCC_VERSION_MINOR<9
    stlwriter.SetCoefficient(5e-5);
#endif
    stlwriter.Write(shape(), filename.c_str());
  }
  else
  {
    throw insight::Exception("Unknown export file format! (Extension "+ext+")");
  }
}

void Feature::exportSTL(const boost::filesystem::path& filename, double abstol) const
{
  TopoDS_Shape os=shape();
  
  ShapeFix_ShapeTolerance sf;
  sf.SetTolerance(os, abstol);
  
  BRepMesh_IncrementalMesh binc(os, abstol);
  
  StlAPI_Writer stlwriter;

  stlwriter.ASCIIMode() = false;
#if OCC_VERSION_MINOR<9
#warning control STL tolerance in newer OCC versions!
  stlwriter.RelativeMode()=false;
  stlwriter.SetDeflection(abstol);
#endif
  stlwriter.Write(shape(), filename.c_str());
}


void Feature::exportEMesh
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
  
  BOOST_FOREACH(const FeatureID& fi, fs.data())
  {
    
    TopoDS_Edge e=fs.model()->edge(fi);
    if (!BRep_Tool::Degenerated(e))
    {
//       std::cout<<"feature "<<fi<<std::endl;
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

	arma::mat p=insight::Vector(pp);
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


Feature::operator const TopoDS_Shape& () const 
{ 
  return shape(); 
}

const TopoDS_Shape& Feature::shape() const
{
  if (building())
    throw insight::Exception("Internal error: recursion during build!");
  checkForBuildDuringAccess();
  if (visresolution_)
  {
//     Bnd_Box box;
//     BRepMesh_FastDiscret m
//     (
//       visresolution_->value(), 
//       0.5, box, 
//       true, false, false, false
//     );
//     m.Perform(shape_);  
    BRepMesh_IncrementalMesh aMesher(shape_, visresolution_->value());
  }
  return shape_;
}


Feature::View Feature::createView
(
    const arma::mat p0,
    const arma::mat n,
    bool section,
    const arma::mat up,
    bool poly,
    bool skiphl
) const
{
    View result_view;

    TopoDS_Shape dispshape=shape();

    gp_Pnt p_base = gp_Pnt(p0(0), p0(1), p0(2));
    gp_Dir view_dir = -gp_Dir(n(0), n(1), n(2));

    gp_Ax2 viewCS(p_base, view_dir);
    if (up.n_elem==3)
    {
        arma::mat ex = cross(up, n);
        viewCS=gp_Ax2(p_base, -view_dir, gp_Dir(to_Vec(ex)));
    }

    HLRAlgo_Projector projector( viewCS );
    gp_Trsf transform=projector.FullTransformation();

    if (section)
    {
        gp_Dir normal = -view_dir;
        gp_Pln plane = gp_Pln(p_base, normal);
        gp_Pnt refPnt = gp_Pnt(p_base.X()+normal.X(), p_base.Y()+normal.Y(), p_base.Z()+normal.Z());

        TopoDS_Face Face = BRepBuilderAPI_MakeFace(plane);
        TopoDS_Shape HalfSpace = BRepPrimAPI_MakeHalfSpace(Face,refPnt).Solid();

        TopoDS_Compound dispshapes;
        boost::shared_ptr<TopTools_ListOfShape> xsecs(new TopTools_ListOfShape);
        BRep_Builder builder1;
        builder1.MakeCompound( dispshapes );
        int i=-1, j=0;
        for (TopExp_Explorer ex(shape(), TopAbs_SOLID); ex.More(); ex.Next())
        {
            TopoDS_Compound cxsecs;
            BRep_Builder builder2;
            builder2.MakeCompound( cxsecs );
            i++;
            try
            {
                builder1.Add(dispshapes, 	BRepAlgoAPI_Cut(ex.Current(), HalfSpace));
                builder2.Add(cxsecs, 		BRepBuilderAPI_Transform(BRepAlgoAPI_Common(ex.Current(), Face), transform).Shape());
                j++;
            }
            catch (...)
            {
                cout<<"Warning: Failed to compute cross section of solid #"<<i<<endl;
            }
            xsecs->Append(cxsecs);
        }
//         cout<<"Generated "<<j<<" cross-sections"<<endl;
        dispshape=dispshapes;
// 	BRepTools::Write(dispshape, "dispshape.brep");
// 	BRepTools::Write(xsecs, "xsecs.brep");
        result_view.crossSections = xsecs;
    }

    TopoDS_Compound allVisible, allHidden;
//     TopoDS_Shape HiddenEdges;
    BRep_Builder builder, builderh;
    builder.MakeCompound( allVisible );
    builderh.MakeCompound( allHidden );

    if (poly)
    {

        // extracting the result sets:
        Handle_HLRBRep_PolyAlgo aHlrPolyAlgo = new HLRBRep_PolyAlgo();
        HLRBRep_PolyHLRToShape shapes;
//         std::cout<<"TolCoef="<<aHlrPolyAlgo->TolCoef()<<endl;
        if (visresolution_)
            aHlrPolyAlgo->TolCoef(visresolution_->value());
        aHlrPolyAlgo->Load(dispshape);
        aHlrPolyAlgo->Projector(projector);
        aHlrPolyAlgo->Update();
        shapes.Update(aHlrPolyAlgo);

        TopoDS_Shape vs=shapes.VCompound();
        if (!vs.IsNull()) builder.Add(allVisible, vs);
        TopoDS_Shape r1vs=shapes.Rg1LineVCompound();
        if (!r1vs.IsNull()) builder.Add(allVisible, r1vs);
        TopoDS_Shape olvs = shapes.OutLineVCompound();
        if (!olvs.IsNull()) builder.Add(allVisible, olvs);

//         HiddenEdges = shapes.HCompound();
        TopoDS_Shape hs = shapes.HCompound();
        if (!hs.IsNull()) builderh.Add(allHidden, hs);
        TopoDS_Shape r1hs=shapes.Rg1LineHCompound();
        if (!r1hs.IsNull()) builderh.Add(allHidden, r1hs);
        TopoDS_Shape olhs = shapes.OutLineHCompound();
        if (!olhs.IsNull()) builderh.Add(allHidden, olhs);

    }
    else
    {
        Handle_HLRBRep_Algo brep_hlr = new HLRBRep_Algo;
        brep_hlr->Add( dispshape );
        brep_hlr->Projector( projector );
        brep_hlr->Update();
        brep_hlr->Hide();
        HLRBRep_HLRToShape shapes( brep_hlr );

        TopoDS_Shape vs=shapes.VCompound();
        if (!vs.IsNull()) builder.Add(allVisible, vs);
        TopoDS_Shape r1vs=shapes.Rg1LineVCompound();
        if (!r1vs.IsNull()) builder.Add(allVisible, r1vs);
        TopoDS_Shape olvs = shapes.OutLineVCompound();
        if (!olvs.IsNull()) builder.Add(allVisible, olvs);

        TopoDS_Shape hs = shapes.HCompound();
        if (!hs.IsNull()) builderh.Add(allHidden, hs);
        TopoDS_Shape r1hs=shapes.Rg1LineHCompound();
        if (!r1hs.IsNull()) builderh.Add(allHidden, r1hs);
        TopoDS_Shape olhs = shapes.OutLineHCompound();
        if (!olhs.IsNull()) builderh.Add(allHidden, olhs);
    }

    result_view.visibleEdges=allVisible;
    if (!skiphl)
    {
        result_view.hiddenEdges=allHidden;
    }
    
    
    BRepMesh_IncrementalMesh Inc(allVisible, 0.0001);
    Bnd_Box boundingBox;
    BRepBndLib::Add(allVisible, boundingBox);

    arma::mat x=arma::zeros(3,2);
    boundingBox.Get
    (
        x(0,0), x(1,0), x(2,0), 
        x(0,1), x(1,1), x(2,1)
    );
    
    result_view.drawing_ctr_x=0.5*(x(0,1)+x(0,0));
    result_view.drawing_ctr_y=0.5*(x(1,1)+x(1,0));
    result_view.width=x(0,1)-x(0,0);
    result_view.height=x(1,1)-x(1,0);

    return result_view;

}

void Feature::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "asModel",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ( ruleset.r_vertexFeaturesExpression | ruleset.r_edgeFeaturesExpression | ruleset.r_faceFeaturesExpression | ruleset.r_solidFeaturesExpression ) >> ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Feature>(qi::_1)) ]
      
    ))
  );
}

FeatureCmdInfoList Feature::ruleDocumentation() const
{
    if (type() == Feature::typeName)
    {
        return boost::assign::list_of
        (
            FeatureCmdInfo
            (
                "asModel",
                "( <verticesSelection>|<edgesSelection>|<facesSelection>|<solidSelection> )",
                "Creates a new feature from selected entities of an existing feature."
            )
        );
    }
    else
    {
        return FeatureCmdInfoList();
    }
}

bool Feature::isSingleEdge() const
{
  return false;
}

bool Feature::isSingleFace() const
{
  return false;
}

bool Feature::isSingleOpenWire() const
{
  return false;
}

bool Feature::isSingleClosedWire() const
{
  return false;
}

bool Feature::isSingleWire() const
{
  return (isSingleOpenWire() || isSingleClosedWire());
}


bool Feature::isSingleVolume() const
{
  return false;
}

TopoDS_Edge Feature::asSingleEdge() const
{
  if (!isSingleEdge())
    throw insight::Exception("Feature "+type()+" does not provide a single edge!");
  else
    return TopoDS::Edge(shape());
}

TopoDS_Face Feature::asSingleFace() const
{
  if (!isSingleFace())
    throw insight::Exception("Feature "+type()+" does not provide a single face!");
  else
    return TopoDS::Face(shape());
}

TopoDS_Wire Feature::asSingleOpenWire() const
{
  if (!isSingleOpenWire())
    throw insight::Exception("Feature "+type()+" does not provide a single open wire!");
  else
  {
    return asSingleWire();
  }
}

TopoDS_Wire Feature::asSingleClosedWire() const
{
  if (!isSingleClosedWire())
    throw insight::Exception("Feature "+type()+" does not provide a single closed wire!");
  else
  {
    return asSingleWire();
  }
}

TopoDS_Wire Feature::asSingleWire() const
{
  if (isSingleWire())
    return TopoDS::Wire(shape());
  else
    throw insight::Exception("Feature "+type()+" does not provide a single wire!");
}


TopoDS_Shape Feature::asSingleVolume() const
{
  if (!isSingleVolume())
    throw insight::Exception("Feature "+type()+" does not provide a single volume!");
  else
    return shape();
}




void Feature::copyDatums(const Feature& m1, const std::string& prefix, std::set<std::string> excludes)
{
    // Transform all ref points and ref vectors
    BOOST_FOREACH(const RefValuesList::value_type& v, m1.getDatumScalars())
    {
        if (excludes.find(v.first)==excludes.end())
        {
            if (refvalues_.find(prefix+v.first)!=refvalues_.end())
                throw insight::Exception("datum value "+prefix+v.first+" already present!");
            refvalues_[prefix+v.first]=v.second;
        }
    }
    BOOST_FOREACH(const RefPointsList::value_type& p, m1.getDatumPoints())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refpoints_.find(prefix+p.first)!=refpoints_.end())
                throw insight::Exception("datum point "+prefix+p.first+" already present!");
            refpoints_[prefix+p.first]=p.second;
        }
    }
    BOOST_FOREACH(const RefVectorsList::value_type& p, m1.getDatumVectors())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refvectors_.find(prefix+p.first)!=refvectors_.end())
                throw insight::Exception("datum vector "+prefix+p.first+" already present!");
            refvectors_[prefix+p.first]=p.second;
        }
    }
    BOOST_FOREACH(const SubfeatureMap::value_type& sf, m1.providedSubshapes())
    {
        if (excludes.find(sf.first)==excludes.end())
        {
            if (providedSubshapes_.find(prefix+sf.first)!=providedSubshapes_.end())
                throw insight::Exception("subshape "+prefix+sf.first+" already present!");
            providedSubshapes_[prefix+sf.first]=sf.second;
        }
    }
    BOOST_FOREACH(const DatumPtrMap::value_type& df, m1.providedDatums())
    {
        if (excludes.find(df.first)==excludes.end())
        {
            if (providedDatums_.find(prefix+df.first)!=providedDatums_.end())
                throw insight::Exception("datum "+prefix+df.first+" already present!");
            providedDatums_[prefix+df.first]=df.second;
        }
    }

}




void Feature::copyDatumsTransformed(const Feature& m1, const gp_Trsf& trsf, const std::string& prefix, std::set<std::string> excludes)
{
    // Transform all ref points and ref vectors
    BOOST_FOREACH(const RefValuesList::value_type& v, m1.getDatumScalars())
    {
        if (excludes.find(v.first)==excludes.end())
        {
            if (refvalues_.find(prefix+v.first)!=refvalues_.end())
                throw insight::Exception("datum value "+prefix+v.first+" already present!");
            refvalues_[prefix+v.first]=v.second;
        }
    }
    BOOST_FOREACH(const RefPointsList::value_type& p, m1.getDatumPoints())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refpoints_.find(prefix+p.first)!=refpoints_.end())
                throw insight::Exception("datum point "+prefix+p.first+" already present!");
            refpoints_[prefix+p.first]=vec3(to_Pnt(p.second).Transformed(trsf));
        }
    }
    BOOST_FOREACH(const RefVectorsList::value_type& p, m1.getDatumVectors())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refvectors_.find(prefix+p.first)!=refvectors_.end())
                throw insight::Exception("datum vector "+prefix+p.first+" already present!");
            refvectors_[prefix+p.first]=vec3(to_Vec(p.second).Transformed(trsf));
        }
    }
    BOOST_FOREACH(const SubfeatureMap::value_type& sf, m1.providedSubshapes())
    {
        if (excludes.find(sf.first)==excludes.end())
        {
            if (providedSubshapes_.find(prefix+sf.first)!=providedSubshapes_.end())
                throw insight::Exception("subshape "+prefix+sf.first+" already present!");
            providedSubshapes_[prefix+sf.first]=FeaturePtr(new Transform(sf.second, trsf));
        }
    }
    BOOST_FOREACH(const DatumPtrMap::value_type& df, m1.providedDatums())
    {
        if (excludes.find(df.first)==excludes.end())
        {
            if (providedDatums_.find(prefix+df.first)!=providedDatums_.end())
                throw insight::Exception("datum "+prefix+df.first+" already present!");
            providedDatums_[prefix+df.first]=DatumPtr(new TransformedDatum(df.second, trsf));
        }
    }
}




const Feature::RefValuesList& Feature::getDatumScalars() const
{
  checkForBuildDuringAccess();
  return refvalues_;
}

const Feature::RefPointsList& Feature::getDatumPoints() const
{
  checkForBuildDuringAccess();
  return refpoints_;
}

const Feature::RefVectorsList& Feature::getDatumVectors() const
{
  checkForBuildDuringAccess();
  return refvectors_;
}

double Feature::getDatumScalar(const std::string& name) const
{
  checkForBuildDuringAccess();
  RefValuesList::const_iterator i = refvalues_.find(name);
  if (i!=refvalues_.end())
  {
    return i->second;
  }
  else
  {
    std::ostringstream av;
    BOOST_FOREACH(const RefValuesList::value_type& i, refvalues_)
    {
      av<<" "<<i.first;
    }
    throw insight::Exception("the feature does not define a reference value named \""+name+"\". Available:"+av.str());
    return 0.0;
  }
}

arma::mat Feature::getDatumPoint(const std::string& name) const
{
  checkForBuildDuringAccess();
  RefPointsList::const_iterator i = refpoints_.find(name);
  if (i!=refpoints_.end())
  {
    return i->second;
  }
  else
  {
    std::ostringstream av;
    BOOST_FOREACH(const RefPointsList::value_type& i, refpoints_)
    {
      av<<" "<<i.first;
    }
    throw insight::Exception("the feature does not define a reference point named \""+name+"\". Available:"+av.str());
    return arma::mat();
  }
}

arma::mat Feature::getDatumVector(const std::string& name) const
{
  checkForBuildDuringAccess();
  RefVectorsList::const_iterator i = refvectors_.find(name);
  if (i!=refvectors_.end())
  {
    return i->second;
  }
  else
  {
    std::ostringstream av;
    BOOST_FOREACH(const RefVectorsList::value_type& i, refvectors_)
    {
      av<<" "<<i.first;
    }
    throw insight::Exception("the feature does not define a reference vector named \""+name+"\". Available:"+av.str());
    return arma::mat();
  }
}

void Feature::nameFeatures()
{
  // Don't call "shape()" here!
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
  
//   extractReferenceFeatures();
}

void Feature::extractReferenceFeatures()
{
  ///////////////////////////////////////////////////////////////////////////////
  /////////////// save reference points

  for (int i=1; i<=vmap_.Extent(); i++)
  {
    refpoints_[ str(format("v%d")%i) ] = vertexLocation(i);
  }
}

void Feature::write(const filesystem::path& file) const
{
  std::ofstream f(file.c_str());
  write(f);
}


void Feature::write(std::ostream& f) const
{
  f<<isleaf_<<endl;

  // the shape
  {
    std::ostringstream bufs;
    BRepTools::Write(shape(), bufs);
    std::string buf=bufs.str();
//     cout<<buf<<endl;
    f<<buf.size()<<endl;
    f<<buf<<endl;
  }

//   nameFeatures();

//   f<<providedSubshapes_.size()<<endl;
//   BOOST_FOREACH(const Feature::Map::value_type& i, providedSubshapes_)
//   {
//     f<<i.first<<endl;
//     i.second->write(f);
//     f<<endl;
//   }

//   typedef std::map<std::string, boost::shared_ptr<Datum> > DatumMap;
//   f<<providedDatums_.size()<<endl;
//   BOOST_FOREACH(const DatumMap::value_type& i, providedDatums_)
//   {
//     f<<i.first<<endl;
//     i.second->write(f);
//     f<<endl;
//   }


//   RefValuesList refvalues_;
  f<<refvalues_.size()<<endl;
  BOOST_FOREACH(const RefValuesList::value_type& i, refvalues_)
  {
    f<<i.first<<endl;
    f<<i.second<<endl;
  }
//   RefPointsList refpoints_;
  f<<refpoints_.size()<<endl;
  BOOST_FOREACH(const RefPointsList::value_type& i, refpoints_)
  {
    f<<i.first<<endl;
    f<<i.second(0)<<" "<<i.second(1)<<" "<<i.second(2)<<endl;
  }
//   RefVectorsList refvectors_;
  f<<refvectors_.size()<<endl;
  BOOST_FOREACH(const RefVectorsList::value_type& i, refvectors_)
  {
    f<<i.first<<endl;
    f<<i.second(0)<<" "<<i.second(1)<<" "<<i.second(2)<<endl;
  }

  f<<density()<<endl;
  f<<areaWeight()<<endl;
}

void Feature::read(const filesystem::path& file)
{
  cout<<"Reading model of type "<<type()<<" from cache file "<<file<<endl;
  std::ifstream f(file.c_str());
  read(f);
}

bool Feature::isTransformationFeature() const
{
  return false;
}

gp_Trsf Feature::transformation() const
{
  throw insight::Exception("not implemented");
  return gp_Trsf();
}

Mass_CoG_Inertia compoundProps(const std::vector<boost::shared_ptr<Feature> >& feats, double density_ovr, double aw_ovr)
{
  double m=0.0;
  arma::mat cog=vec3(0,0,0);
  
  double mcs[feats.size()];
  arma::mat cogs[feats.size()];
  
  int i;
  i=-1; BOOST_FOREACH(const FeaturePtr& f, feats)
  { i++;
    
//     std::cout<<density_ovr<<", "<<aw_ovr<<std::endl;
    mcs[i]=f->mass(density_ovr, aw_ovr);
    cogs[i]=f->modelCoG(density_ovr);
    
//     std::cout<<"m="<<mc<<", cog="<<f->modelCoG()<<std::endl;
    m += mcs[i];
    cog += cogs[i]*mcs[i];
  }
  cog/=m;
  
  arma::mat inertia=arma::zeros(3,3);
  i=-1; BOOST_FOREACH(const FeaturePtr& f, feats)
  { i++;
    arma::mat d = cogs[i] - cog;
    arma::mat dt=arma::zeros(3,3);
    dt(0,1)=-d(2);
    dt(0,2)=d(1);
    dt(1,2)=-d(0);
    dt(1,0)=d(2);
    dt(2,0)=-d(1);
    dt(2,1)=d(0);
    
    inertia += f->modelInertia(density_ovr) + mcs[i]*dt.t()*dt;
  }
  
//   std::cout<<"compound props: m="<<m<<", cog="<<cog<<std::endl;
  return Mass_CoG_Inertia(m, cog, inertia);
}

arma::mat rotTrsf(const gp_Trsf& tr)
{
  gp_Mat m=tr.VectorialPart();
  
  arma::mat R=arma::zeros(3,3);
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      R(i,j)=m.Value(i+1,j+1);
    
  return R;
}

arma::mat transTrsf(const gp_Trsf& tr)
{
  gp_XYZ v=tr.TranslationPart();
  return vec3( v.X(), v.Y(), v.Z() );
}

void Feature::read(std::istream& f)
{
  int n;
  
  f>>isleaf_;

  {
    size_t s;
    f>>s;
    
    char buf[s+2];
    f.read(buf, s);
    buf[s]='\0';
//     cout<<buf<<endl;
    
    BRep_Builder b;
    std::istringstream bufs(buf);
    TopoDS_Shape sh;
    BRepTools::Read(sh, bufs, b);
    setShape(sh);
  }

//   f<<providedSubshapes_.size()<<endl;
//   BOOST_FOREACH(const Feature::Map::value_type& i, providedSubshapes_)
//   {
//     f<<i.first<<endl;
//     i.second->write(f);
//     f<<endl;
//   }

//   typedef std::map<std::string, boost::shared_ptr<Datum> > DatumMap;
//   int n;
// 
//   f>>n;
//   for (int i=0; i<n; i++)
//   {
//     std::string name;
//     getline(f, name);
//     providedDatums_[name].reset(new Datum(f));
//   }


//   RefValuesList refvalues_;
  f>>n;
  cout<<"reading "<<n<<" ref values"<<endl;
  for (int i=0; i<n; i++)
  {
    std::string name;
//     getline(f, name);
    f>>name;
    double v;
    f>>v;
    cout<<name<<": "<<v<<endl;
    refvalues_[name]=v;
  }
//   RefPointsList refpoints_;
  f>>n;
  cout<<"reading "<<n<<" ref points"<<endl;
  for (int i=0; i<n; i++)
  {
    std::string name;
//     getline(f, name);
    f>>name;
    double x, y, z;
    f>>x>>y>>z;
    cout<<name<<": "<<x<<" "<<y<<" "<<z<<endl;
    refpoints_[name]=vec3(x, y, z);
  }
//   RefVectorsList refvectors_;
  f>>n;
  cout<<"reading "<<n<<" ref vectors"<<endl;
  for (int i=0; i<n; i++)
  {
    std::string name;
//     getline(f, name);
    f>>name;
    double x, y, z;
    f>>x>>y>>z;
    refvectors_[name]=vec3(x, y, z);
  }

//   double density_, areaWeight_;

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
  return BRepTools::OuterWire(TopoDS::Face(shape()));;
}



bool SingleVolumeFeature::isSingleVolume() const
{
  return true;
}

// FeatureCache::FeatureCache(const filesystem::path& cacheDir)
// : cacheDir_(cacheDir),
//   removeCacheDir_(false)
// {
//   if (cacheDir.empty())
//   {
//     removeCacheDir_=true;
//     cacheDir_ = boost::filesystem::unique_path
//     (
//       boost::filesystem::temp_directory_path()/("iscad_cache_%%%%%%%")
//     );
// //     boost::filesystem::create_directories(cacheDir_);
//   }
// }
// 
// FeatureCache::~FeatureCache()
// {
// //   if (removeCacheDir_)
// //   {
// //     boost::filesystem::remove_all(cacheDir_);
// //   }
// }
// 
// void FeatureCache::initRebuild()
// {
// //   usedFilesDuringRebuild_.clear();
// }
// 
// void FeatureCache::finishRebuild()
// {
//   // remove all cache files that have not been used
// }
// 
// 
// bool FeatureCache::contains(size_t hash) const
// {
// //   return boost::filesystem::exists(fileName(hash));
//   return false;
// }
// 
// 
// filesystem::path FeatureCache::markAsUsed(size_t hash)
// {
// //   usedFilesDuringRebuild_.insert(fileName(hash));
//   return fileName(hash);
// }
// 
// filesystem::path FeatureCache::fileName(size_t hash) const
// {
//   return boost::filesystem::absolute
//   (
//     cacheDir_ /
//     boost::filesystem::path( str(format("%x")%hash) + ".iscad_cache" )
//   );
// }


FeatureCache::FeatureCache()
{}

FeatureCache::~FeatureCache()
{}

void FeatureCache::initRebuild()
{
   usedDuringRebuild_.clear();
}

void FeatureCache::finishRebuild()
{
  // remove all cache entries that have not been used
  std::cout<<"== Finish Rebuild: Cache Summary =="<<std::endl;
  std::cout<<"cache size after rebuild: "<<size()<<std::endl;
  std::cout<<"# used during rebuild: "<<usedDuringRebuild_.size()<<std::endl;
  
  for (auto it = cbegin(); it != cend();)
  {
    if ( usedDuringRebuild_.find(it->first)==usedDuringRebuild_.end() )
    {
      erase(it++);
    }
    else
    {
      ++it;
    }
  }
  std::cout<<"cache size after cleanup: "<<size()<<std::endl;
}

void FeatureCache::insert(FeaturePtr p)
{
  size_t h=p->hash();
  if (find(h)!=end())
      throw insight::Exception("Internal error: trying to insert existing feature into CAD feature cache!");
  (*this)[h]=p;
  usedDuringRebuild_.insert(h);
}


bool FeatureCache::contains(size_t hash) const
{
  return ( this->find(hash) != end() );
}



FeatureCache cache;

}
}
