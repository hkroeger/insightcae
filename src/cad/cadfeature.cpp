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
#include "cadfeatures/transform.h"
#include "base/tools.h"

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
//#include "XCAFApp_Application.hxx"
//#include "XCAFDoc.hxx"
//#include "XCAFDoc_DocumentTool.hxx"
//#include "XCAFDoc_ShapeTool.hxx"
//#include "XSControl_WorkSession.hxx"
//#include "XSControl_TransferReader.hxx"
//#include "XSControl_TransferWriter.hxx"
#include "StepData_StepModel.hxx"
#include "TDF_LabelSequence.hxx"
#if (OCC_VERSION_MAJOR>=7)
#else
#include "Handle_StepRepr_RepresentationItem.hxx"
#endif
//#include "STEPConstruct.hxx"
//#include "STEPCAFControl_Writer.hxx"
//#include "STEPCAFControl_Reader.hxx"
#include "StepRepr_RepresentationItem.hxx"
#include "StepShape_AdvancedFace.hxx"
#include "APIHeaderSection_MakeHeader.hxx"
#include "TDataStd_Name.hxx"
#include "TDF_ChildIterator.hxx"
#include "TDF_ChildIDIterator.hxx"
#include "TransferBRep.hxx"
#include "Transfer_Binder.hxx"
#include "Transfer_TransientProcess.hxx"
#include "TopTools_DataMapIteratorOfDataMapOfIntegerShape.hxx"

#include "openfoam/openfoamdict.h"

#include "TColStd_SequenceOfTransient.hxx"
#include "TColStd_HSequenceOfTransient.hxx"

#include "BRepBuilderAPI_Copy.hxx"

#if (OCC_VERSION_MAJOR<7)
#include "compat/Bnd_OBB.hxx"
#include "compat/BRepBndLib.hxx"
#else
#include "Bnd_OBB.hxx"
#include "BRepBndLib.hxx"
#endif

#include "base/parameters/pathparameter.h"

#include "featurefilters/same.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace boost
{


std::size_t hash<TopoDS_Shape>::operator()(const TopoDS_Shape& shape) const
{
    auto m = insight::cad::Feature::create(shape);
    return m->hash();
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


std::size_t hash<gp_Trsf>::operator()(const gp_Trsf& t) const
{
  std::hash<double> dh;
  size_t h=0;
  for (int c=1; c<=4; c++)
  {
      for (int r=1; r<=3; r++)
      {
          boost::hash_combine(h, dh(t.Value(r, c)));
      }
  }
  boost::hash_combine(h, dh(t.ScaleFactor()));
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
: hash_(0)
{}

size_t ParameterListHash::getHash() const
{
  return hash_;
}



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


void FreelyIndexedMapOfShape::Add(const TopoDS_Shape& s, int index)
{
    // ensure that entity is only contained once
    int i=FindIndex(s);
    if (i>0) erase(find(i));
    
    if (index<=0) index=getMaxIndex()+1;
    
    (*this)[index]=s;
}

bool FreelyIndexedMapOfShape::contains (const TopoDS_Shape& K)  const
{
    return FindIndex(K)>0;
}

const TopoDS_Shape& FreelyIndexedMapOfShape::FindKey (const Standard_Integer I)  const
{
    const_iterator i=find(I);
    if (i==end())
        throw insight::Exception(boost::str(boost::format("No shape with tag %d") % I));
    return i->second;
}

const  TopoDS_Shape& FreelyIndexedMapOfShape::operator() (const Standard_Integer I)  const
{
    return FindKey(I);
} 

int FreelyIndexedMapOfShape::FindIndex (const TopoDS_Shape& K)  const
{
    if (size()==0) return -1;
    
    int k=-1;
    for (const value_type& i: *this)
    {
        if (K.IsSame(i.second))
        {
            if (k>=0)
            {
                insight::Warning(
                    boost::str(boost::format("Multiple keys for shape! (at least at %d and %d)") % k % i.first)
                );
//                 return k;
            }
//             else
//             {
                k=i.first;
//             }
        }
    }
    return k;
}

int FreelyIndexedMapOfShape::getMaxIndex() const
{
    if (size()==0)
    {
        return 0;
    }
    else
    {
        return rbegin()->first;
    }
}
    

defineType(Feature);
//defineFactoryTableNoArgs(Feature);
//addToFactoryTable(Feature, Feature);

defineStaticFunctionTableWithArgs(
    Feature,
    insertrule,
    void,
    LIST(insight::cad::parser::ISCADParser& ruleset),
    LIST(ruleset) );

defineStaticFunctionTable(
    Feature,
    ruleDocumentation,
    FeatureCmdInfoList );

std::mutex Feature::step_read_mutex_;

void Feature::loadShapeFromFile(const boost::filesystem::path& filename)
{
    ParameterListHash h;
    h.addParameter(this->type());
    h.addParameter(filename);
    hash_=h.getHash();

    std::string ext=filename.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext==".brep")
    {
        BRep_Builder bb;
        TopoDS_Shape s;
        BRepTools::Read(s, filename.string().c_str(), bb);
        BRepTools::Clean(s);
        setShape(s);
    }
    else if ( (ext==".igs") || (ext==".iges") )
    {
        IGESControl_Controller::Init();
        Interface_Static::SetIVal("read.surfacecurve.mode",3);
//        Interface_Static::SetIVal ("read.precision.mode",1);
//        Interface_Static::SetRVal("read.precision.val",0.001);

        IGESControl_Reader igesReader;

        igesReader = IGESControl_Reader();
        igesReader.SetReadVisible( true );
        igesReader.ReadFile(filename.string().c_str());
        igesReader.PrintCheckLoad(false, IFSelect_ItemsByEntity);

        igesReader.TransferRoots();

        setShape(igesReader.OneShape());
    }
    else if ( (ext==".stp") || (ext==".step") )
    {
        TopoDS_Shape res;
        {
          std::lock_guard<std::mutex> guard(step_read_mutex_);

          // import STEP
          STEPControl_Reader reader;
          reader.ReadFile(filename.string().c_str());
          reader.TransferRoots();

          res=reader.OneShape();
        }
        // set shape
        setShape(res);

//        // now detect named features
//        //
//        cout<<"detecting names"<<endl;

//        typedef std::map<std::string, FeatureSetPtr> Feats;
//        Feats feats;

//        Handle_TColStd_HSequenceOfTransient shapeList = reader.GiveList("xst-model-all");
//        reader.TransferList(shapeList);


//        const Handle_XSControl_WorkSession & theSession = reader.WS();
//        const Handle_XSControl_TransferReader & aReader = theSession->TransferReader();
//        const Handle_Transfer_TransientProcess & tp = aReader->TransientProcess();

//        for(int i=1; i <= shapeList->Length(); i++)
//        {
//            Handle_Standard_Transient transient = shapeList->Value(i);
//            TopoDS_Shape shape = TransferBRep::ShapeResult(tp, transient);
//            if(!shape.IsNull())
//            {
//                Handle_Standard_Transient anEntity = aReader->EntityFromShapeResult(shape, 1);
//                if(!anEntity.IsNull())
//                {
//                    Handle_StepRepr_RepresentationItem entity = Handle_StepRepr_RepresentationItem::DownCast(anEntity);
//                    if(!entity.IsNull())
//                    {
//                        if (!entity->Name()->IsEmpty())  // found named entity
//                        {
//                            std::string n(entity->Name()->ToCString());
//                            if (shape.ShapeType()==TopAbs_FACE)
//                            {
//                                std::vector<FeatureID> ids;
//                                for(TopExp_Explorer ex(res, TopAbs_FACE); ex.More(); ex.Next())
//                                {
//                                    TopoDS_Face f=TopoDS::Face(ex.Current());
//                                    if (f.IsPartner(shape))
//                                    {
//                                        ids.push_back(faceID(f));
////                                        std::cout<<"MATCH! face id="<<(ids.back())<<std::endl;
//                                    }
//                                }
//                                if (ids.size()==0)
//                                {
//                                    insight::Warning("could not identify named face in model! (face named \""+n+"\")");
//                                }
//                                else
//                                {
//                                    std::string name="face_"+n;
//                                    if (feats.find(name)==feats.end())
//                                        feats[name].reset(new FeatureSet(shared_from_this(), Face));
//                                    for (const FeatureID& i: ids)
//                                    {
//                                        feats[name]->add(i);
//                                    }
//                                }
//                            }
//                            else if (shape.ShapeType()==TopAbs_SOLID)
//                            {
//                                std::vector<FeatureID> ids;
//                                for(TopExp_Explorer ex(res, TopAbs_SOLID); ex.More(); ex.Next())
//                                {
//                                    TopoDS_Solid f=TopoDS::Solid(ex.Current());
//                                    if (f.IsPartner(shape))
//                                    {
//                                        ids.push_back(solidID(f));
////                                        std::cout<<"MATCH! solid id="<<ids.back()<<std::endl;
//                                    }
//                                }
//                                if (ids.size()==0)
//                                {
//                                    insight::Warning("could not identify named solid in model! (solid named \""+n+"\")");
//                                }
//                                else
//                                {
//                                    std::string name="solid_"+n;
//                                    if (feats.find(name)==feats.end())
//                                        feats[name].reset(new FeatureSet(shared_from_this(), Solid));
//                                    for (const FeatureID& i: ids)
//                                    {
//                                        feats[name]->add(i);
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }


//        for (Feats::value_type& f: feats)
//        {
//            providedFeatureSets_[f.first]=f.second;
//            providedSubshapes_[f.first].reset(new Feature(f.second));
//        }

    }
    else
    {
        throw insight::Exception("Unknown import file format! (Extension "+ext+")");
//     return TopoDS_Shape();
    }
}

size_t Feature::calcShapeHash() const
{
  // create hash from
  // 1. total volume
  // 2. # vertices
  // 3. # faces
  // 4. vertex locations

  size_t hash=0;
  
  boost::hash_combine(hash, boost::hash<double>()(modelVolume()));
  boost::hash_combine(hash, boost::hash<int>()(idx_->nVertexTags()));
  boost::hash_combine(hash, boost::hash<int>()(idx_->nFaceTags()));

  FeatureSetData vset=allVerticesSet();
  for (const insight::cad::FeatureID& j: vset)
  {
    boost::hash_combine
    (
      hash,
      boost::hash<arma::mat>()(vertexLocation(j))
    );    
  }

  return hash;
}




size_t Feature::calcHash() const
{
  ParameterListHash h;
  if (creashapes_) h+=*creashapes_;
  return h.getHash();
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
  featureSymbolName_()
{
}




Feature::Feature(const Feature& o)
: ASTBase(o),
  isleaf_(true),
  providedSubshapes_(o.providedSubshapes_),
  providedFeatureSets_(o.providedFeatureSets_),
  providedDatums_(o.providedDatums_),
  density_(o.density_),
  areaWeight_(o.areaWeight_),
  featureSymbolName_(o.featureSymbolName_)
{
  setShape(o.shape_);
}




Feature::Feature(const TopoDS_Shape& shape)
: isleaf_(true),
  featureSymbolName_()
{
  setShape(shape);
  hash_=calcShapeHash();
  setValid();
}



Feature::Feature(FeatureSetPtr creashapes)
: creashapes_(creashapes),
  featureSymbolName_(/*"subshapesOf_"+creashapes->model()->featureSymbolName()*/)
{}

Feature::~Feature()
{
  auto h=hash();
  if (cache.contains(h))
    cache.erase(h);
}


FeaturePtr Feature::create(const boost::filesystem::path& filepath)
{
  FeaturePtr f(new Feature());
  f->loadShapeFromFile(filepath);
//  f->setShapeHash();
  f->setValid();
  f->setFeatureSymbolName("importedFrom_"+filepath.string());
  return f;
}


void Feature::setFeatureSymbolName( const std::string& name)
{
  featureSymbolName_ = name;
}

bool Feature::isAnonymous() const
{
  return featureSymbolName_.empty();
}

std::string Feature::featureSymbolName() const
{
    return isAnonymous() ?
            ("anonymous_"+type()) :
             featureSymbolName_;
}


void Feature::setVisResolution( ScalarPtr r )
{ 
    visresolution_=r;
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
  
  dbg()<<"rho="<<rho<<" ("<<density_ovr<<")"<<std::endl;
  
  double mtot=rho*modelVolume() + aw*modelSurfaceArea();

  dbg()<<"Computed mass rho / V = "<<rho<<" / "<<modelVolume()
       <<", mf / A = "<<aw<<" / "<<modelSurfaceArea()
       <<", m = "<<mtot<<endl;

  return mtot;
}




void Feature::checkForBuildDuringAccess() const
{
  try
  {
    if (!valid())
      {
        dbg(2)<<"trigger rebuild ["<<featureSymbolName()<<"]"<<std::endl;
      }
    ASTBase::checkForBuildDuringAccess();
  }
  catch (const Standard_Failure& e)
  {
    dbg()<<"exception during feature rebuild"<<std::endl;
    throw insight::cad::CADException(
          shared_from_this(),
          e.GetMessageString() ? e.GetMessageString() : "(no error message)" );
  }
}




void Feature::build()
{
  TopoDS_Compound comp;
  BRep_Builder builder;
  builder.MakeCompound( comp );

  for (const FeatureID& id: creashapes_->data())
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

  if (o.valid())
  {
    if (o.volprops_)
      volprops_.reset(new GProp_GProps(*o.volprops_));

    idx_.reset(new SubshapeNumbering(*o.idx_));
    setValid();
    shape_ = o.shape_;
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


arma::mat Feature::modelCoG(double /*density_ovr*/) const
{
  checkForBuildDuringAccess();
//   GProp_GProps props;
//   TopoDS_Shape sh=shape();
//   BRepGProp::VolumeProperties(sh, props);
  updateVolProps();
  gp_Pnt cog = volprops_->CentreOfMass();
  return vec3(cog);
}

arma::mat Feature::surfaceCoG(double /*areaWeight_ovr*/) const
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
  checkForBuildDuringAccess();
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
  auto dist= dss.Value();
  std::cout<<"dist="<<dist<<std::endl;
  return dist;
}

double Feature::maxVertexDist(const arma::mat& p) const
{
  double maxdist=0.;
  for (TopExp_Explorer ex(shape(), TopAbs_VERTEX); ex.More(); ex.Next())
  {
    TopoDS_Vertex v=TopoDS::Vertex(ex.Current());
    arma::mat vp=insight::Vector(BRep_Tool::Pnt(v));
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
  if (!boundingBox.IsVoid())
  {
    double g=boundingBox.GetGap();
    double x0, x1, y0, y1, z0, z1;

    boundingBox.Get
    (
      x0, y0, z0,
      x1, y1, z1
    );

    x
    << x0 << x1 << arma::endr
    << y0 << y1 << arma::endr
    << z0 << z1 << arma::endr;

    x.col(0)+=g;
    x.col(1)-=g;
  }

  return x;
}

std::pair<CoordinateSystem, arma::mat> Feature::orientedModelBndBox(double deflection) const
{
    checkForBuildDuringAccess();

    if (deflection>0)
    {
        BRepMesh_IncrementalMesh Inc(shape(), deflection);
    }

#if (OCC_VERSION_MAJOR<7)
    ISOCC::Bnd_OBB boundingBox;
    ISOCC::BRepBndLib::AddOBB(shape(), boundingBox);
#else
    Bnd_OBB boundingBox;
    BRepBndLib::AddOBB(shape(), boundingBox);
#endif

    arma::mat x=arma::zeros(3,2);
    CoordinateSystem cs;

    if (!boundingBox.IsVoid())
    {
        arma::mat sizes;
        sizes   << boundingBox.XHSize()
                << boundingBox.YHSize()
                << boundingBox.ZHSize();


//        arma::uvec si = arma::reverse( arma::sort_index(sizes) );
        arma::uvec si=arma::sort_index(sizes);
        std::reverse(si.begin(), si.end());

        cs.origin = insight::Vector(boundingBox.Center());
//        cs.ex = insight::Vector(boundingBox.XDirection());
//        cs.ey = insight::Vector(boundingBox.YDirection());
//        cs.ez = insight::Vector(boundingBox.ZDirection());

        for (int i=0; i<3; ++i)
        {
            arma::mat dr;
            double sz;

            switch (si(i))
            {
            case 0:
                dr=insight::Vector(boundingBox.XDirection());
                sz=boundingBox.XHSize();
                break;
            case 1:
                dr=insight::Vector(boundingBox.YDirection());
                sz=boundingBox.YHSize();
                break;
            case 2:
                dr=insight::Vector(boundingBox.ZDirection());
                sz=boundingBox.ZHSize();
                break;
            };

            switch (i)
            {
            case 0:
                cs.ex=dr;
                break;
            case 1:
                cs.ey=dr;
                break;
            case 2:
                cs.ez=dr;
                break;
            };
            x(i,0)=-sz;
            x(i,1)=sz;
        }
    }

    return {cs, x};
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

arma::mat Feature::averageFaceNormal() const
{
  auto af = allFacesSet();
  insight::assertion(
      af.size()>=1,
      "there are no faces whose normals could be averaged." );
  arma::mat n=vec3Zero();
  for (const auto& i: af)
  {
        n+=faceNormal(i);
  }
  return n/double(af.size());
}

FeatureSetData Feature::allVerticesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
//   fsd.insert(
//     boost::counting_iterator<int>( 1 ),
//     boost::counting_iterator<int>( idx_->_vmap.Extent()+1 )
//   );
  idx_->insertAllVertexTags(fsd);
//  std::transform
//  (
//      vmap_.begin(),
//      vmap_.end(),
//      std::inserter(fsd, fsd.begin()),
//      [](const FreelyIndexedMapOfShape::value_type& i) { return i.first; }
//  );
  return fsd;
}

FeatureSetData Feature::allEdgesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
//   fsd.insert(
//     boost::counting_iterator<int>( 1 ),
//     boost::counting_iterator<int>( idx_->_emap.Extent()+1 )
//   );
  idx_->insertAllEdgeTags(fsd);
//  std::transform
//  (
//      emap_.begin(),
//      emap_.end(),
//      std::inserter(fsd, fsd.begin()),
//      [](const FreelyIndexedMapOfShape::value_type& i) { return i.first; }
//  );
  return fsd;
}

FeatureSetData Feature::allFacesSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  idx_->insertAllFaceTags(fsd);
//   fsd.insert(
//     boost::counting_iterator<int>( 1 ),
//     boost::counting_iterator<int>( idx_->_fmap.Extent()+1 )
//   );
//  std::transform
//  (
//      fmap_.begin(),
//      fmap_.end(),
//      std::inserter(fsd, fsd.begin()),
//      [](const FreelyIndexedMapOfShape::value_type& i) { return i.first; }
//  );
  return fsd;
}

FeatureSetData Feature::allSolidsSet() const
{
  checkForBuildDuringAccess();
  FeatureSetData fsd;
  idx_->insertAllSolidTags(fsd);
//   fsd.insert(
//     boost::counting_iterator<int>( 1 ),
//     boost::counting_iterator<int>( idx_->_somap.Extent()+1 )
//   );
//  std::transform
//  (
//      somap_.begin(),
//      somap_.end(),
//      std::inserter(fsd, fsd.begin()),
//      [](const FreelyIndexedMapOfShape::value_type& i) { return i.first; }
//  );
   return fsd;
}

FeatureSetPtr Feature::allOf(EntityType et) const
{
    switch(et)
    {
    case Vertex:
        return allVertices();
    case Edge:
        return allEdges();
    case Face:
        return allFaces();
    case Solid:
        return allSolids();
    default:
        throw insight::Exception("internal error: unhandled selection");
    }
}

FeatureSetPtr Feature::allVertices() const
{
    checkForBuildDuringAccess();
    auto f=std::make_shared<FeatureSet>(shared_from_this(), Vertex);
    f->setData(allVerticesSet());
    return f;
}

FeatureSetPtr Feature::allEdges() const
{
    checkForBuildDuringAccess();
    auto f=std::make_shared<FeatureSet>(shared_from_this(), Edge);
    f->setData(allEdgesSet());
    return f;
}

FeatureSetPtr Feature::allFaces() const
{
    checkForBuildDuringAccess();
    auto f=std::make_shared<FeatureSet>(shared_from_this(), Face);
    f->setData(allFacesSet());
    return f;
}

FeatureSetPtr Feature::allSolids() const
{
    checkForBuildDuringAccess();
    auto f=std::make_shared<FeatureSet>(shared_from_this(), Solid);
    f->setData(allSolidsSet());
    return f;
}

FeatureSetPtr Feature::find(FeatureSetPtr fs) const
{
    checkForBuildDuringAccess();
    if ( *fs->model() == *this )
    {
        return fs;
    }
    else
    {
        switch (fs->shape())
        {
        case cad::Vertex:
            return std::make_shared<FeatureSet>(
                        shared_from_this(), cad::Vertex,
                        query_vertices(std::make_shared<sameVertex>(*fs)));
        case cad::Edge:
            return std::make_shared<FeatureSet>(
                        shared_from_this(), cad::Edge,
                        query_edges(std::make_shared<sameEdge>(*fs)));
        case cad::Face:
            return std::make_shared<FeatureSet>(
                        shared_from_this(), cad::Face,
                        query_faces(std::make_shared<sameFace>(*fs)));
        case cad::Solid:
            return std::make_shared<FeatureSet>(
                        shared_from_this(), cad::Solid,
                        query_solids(std::make_shared<sameSolid>(*fs)));
        default:
            throw insight::Exception("internal error");
        }
    }
}

FeatureSetData Feature::query_vertices(FilterPtr f) const
{
  return query_vertices_subset(*allVertices(), f);
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
  for (int i: fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  for (int i: fs)
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
  return query_edges_subset(*allEdges(), f);
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
  for (int i: fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  //for (int i=1; i<=emap_.Extent(); i++)
  for (int i: fs)
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
  return query_faces_subset(*allFaces(), f);
}

FeatureSetData Feature::query_faces(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces(parseFaceFilterExpr(is, refs));
}


FeatureSetData Feature::query_faces_subset(const FeatureSetData& fs, FilterPtr f) const
{
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  for (int i: fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  for (int i: fs)
  {
    bool ok=f->checkMatch(i);
    if (ok) res.insert(i);
  }
  insight::dbg(2)<<"QUERY_FACES RESULT = "<<res<<endl;
  return res;
}

FeatureSetData Feature::query_faces_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_faces_subset(fs, parseFaceFilterExpr(is, refs));
}

FeatureSetData Feature::query_solids(FilterPtr f) const
{
  return query_solids_subset(*allSolids(), f);
}

FeatureSetData Feature::query_solids(const string& queryexpr, const FeatureSetParserArgList& refs) const
{
  std::istringstream is(queryexpr);
  return query_solids(parseSolidFilterExpr(is, refs));
}


FeatureSetData Feature::query_solids_subset(const FeatureSetData& fs, FilterPtr f) const
{
  checkForBuildDuringAccess();
  
  f->initialize(shared_from_this());
  for (int i: fs)
  {
    f->firstPass(i);
  }
  FeatureSetData res;
  for (int i: fs)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  insight::dbg(2)<<"QUERY_SOLIDS RESULT = "<<res<<endl;
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
  fsd.insert(idx_->/*_vmap.FindIndex*/tagOfVertex(TopExp::FirstVertex(edge(e))));
  fsd.insert(idx_->/*_vmap.FindIndex*/tagOfVertex(TopExp::LastVertex(edge(e))));
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfEdges(const FeatureSet& es) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  for (FeatureID i: es.data())
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
    fsd.insert(idx_->/*_vmap.FindIndex*/tagOfVertex(TopoDS::Vertex(ex.Current())));
  }
  vertices.setData(fsd);
  return vertices;
}

FeatureSet Feature::verticesOfFaces(const FeatureSet& fs) const
{
  FeatureSet vertices(shared_from_this(), Vertex);
  FeatureSetData fsd;
  for (FeatureID i: fs.data())
  {
    FeatureSet j=verticesOfFace(i);
    fsd.insert(j.data().begin(), j.data().end());
  }
  vertices.setData(fsd);
  return vertices;
}



void Feature::saveAs
(
  const boost::filesystem::path& filename, 
  const std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> >& namedfeats
) const
{
  checkForBuildDuringAccess();
  

  std::string ext=filename.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);


  if (ext==".brep")
  {
    BRepTools::Write(shape(), filename.string().c_str());
  } 


  else if ( (ext==".igs") || (ext==".iges") )
  {
    Interface_Static::SetIVal("write.iges.brep.mode", 1);
    IGESControl_Controller igesctrl;
    igesctrl.Init();
    IGESControl_Writer igeswriter;
    igeswriter.AddShape(shape());
    igeswriter.Write(filename.string().c_str());
  } 


  else if ( (ext==".stp") || (ext==".step") )
  {

   STEPControl_Writer sw;
   sw.Transfer(shape(), STEPControl_AsIs);
   sw.Write(filename.string().c_str());

//   IFSelect_ReturnStatus stat;
   
//   // The various ways of reading a file are available here too :
//   // to read it by the reader, to take it from a WorkSession ...
//   Handle_TDocStd_Document aDoc;
//   {
//      Handle_XCAFApp_Application anApp = XCAFApp_Application::GetApplication();
//      anApp->NewDocument("MDTV-XCAF", aDoc);
//   }

//   Standard_Boolean ok;
//   Handle_XCAFDoc_ShapeTool myAssembly = XCAFDoc_DocumentTool::ShapeTool(aDoc->Main());
//   myAssembly->AddShape(shape());
   
//   STEPControl_StepModelType mode = STEPControl_AsIs;
//   STEPCAFControl_Writer writer;

//   Interface_Static::SetCVal("write.step.schema", "AP214IS");
//   Interface_Static::SetCVal("write.step.product.name", "productInfo" );

//   // configure STEP interface
//   writer.SetColorMode(Standard_True);
//   writer.SetLayerMode(Standard_True);
//   writer.SetNameMode (Standard_True);

//   // Translating document (conversion) to STEP Writer
//   if ( !writer.Transfer ( aDoc, mode ) )
//   {
//      throw insight::Exception("The document cannot be translated or gives no result");
//   }

//   ////// Begin Hack to associate names to Faces/Edges/Vertices /////
//   const Handle_XSControl_WorkSession& theSession = writer.Writer().WS();
//   const Handle_XSControl_TransferWriter& aTransferWriter = theSession->TransferWriter();
//   const Handle_Transfer_FinderProcess FP = aTransferWriter->FinderProcess();
   
//   typedef std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> > FSM;

//   std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> > all_namedfeats = namedfeats;

//   for (const FeatureSetPtrMap::value_type& pfs: providedFeatureSets_)
//   {
//     const std::string& name = pfs.first;
//     const FeatureSetPtr& feat = pfs.second;
//     if ( feat->shape() == Face )
//       {
//         all_namedfeats.push_back( boost::fusion::vector2<std::string, FeatureSetPtr>(name, feat) );
//       }
//   }

//   for (const FSM::value_type& fp: all_namedfeats)
//   {
//     std::string name = boost::fusion::get<0>(fp); // fp.first;
//     const FeatureSetPtr& fs = boost::fusion::get<1>(fp); //fp.second;
     
//     if ( fs->shape() == Face )
//     {
//       for (const FeatureID& id: fs->data())
//       {
//	 TopoDS_Face aFace = fs->model()->face(id);
//	 Handle_StepRepr_RepresentationItem r = STEPConstruct::FindEntity(FP, aFace);
//	 if (r.IsNull() == Standard_False)
//	 {
//	    // cast the StepRepr_RepresentationItem to a StepShape_AdvancedFace of the STEP Writer : variable "x"
//	    Handle_StepShape_AdvancedFace x = Handle_StepShape_AdvancedFace::DownCast(r);
//	    if (x.IsNull() == Standard_True)
//	    {
//	      throw insight::Exception
//	      (
//		"Failed to Down-cast StepRepr_RepresentationItem into "+ std::string(x->DynamicType()->Name())
//	      );
//	    }
	    
//	    Handle(TCollection_HAsciiString) newid = new TCollection_HAsciiString(name.c_str());
//	    x->SetName(newid);
//	 #warning check, if already named?
//	 }
//	 else
//	 {
//	    insight::Warning
//	    (
//	      boost::str(boost::format("Feature set face#%d not found in model")%id)
//	    );
//	 }
//// 	 StepShape_SetName<Handle_StepShape_AdvancedFace> (r, reader, aFace, "My Face", i++);
//       }
//     } else
//     {
//       throw insight::Exception("Given feature set not consisting of faces: yet unsupported");
//     }
//   }
   
//   // edit STEP header
//   APIHeaderSection_MakeHeader makeHeader(writer.ChangeWriter().Model());

//   Handle_TCollection_HAsciiString headerFileName = new TCollection_HAsciiString(filename.stem().string().c_str());
////    Handle(TCollection_HAsciiString) headerAuthor      = new TCollection_HAsciiString("silentdynamics GmbH");
//   Handle_TCollection_HAsciiString headerOrganization = new TCollection_HAsciiString("silentdynamics GmbH");
//   Handle_TCollection_HAsciiString headerOriginatingSystem = new TCollection_HAsciiString("Insight CAD");
//   Handle_TCollection_HAsciiString fileDescription = new TCollection_HAsciiString("iscad model");

//   makeHeader.SetName(headerFileName);
////    makeHeader.SetAuthorValue (1, headerAuthor);
//   makeHeader.SetOrganizationValue (1, headerOrganization);
//   makeHeader.SetOriginatingSystem(headerOriginatingSystem);
//   makeHeader.SetDescriptionValue(1, fileDescription);

//   // Writing the File (With names associated to Faces/Edges/Vertices)
//   ok=Standard_True;
//   stat = writer.Write(filename.string().c_str());
//   if (stat!=IFSelect_RetDone) ok = Standard_False;
   
//   if (!ok) throw insight::Exception("STEP export failed!");
    
  } 

  else if ( (ext==".stl") || (ext==".stlb") )
  {
    BRepMesh_IncrementalMesh Inc(shape(), 1e-2);
    StlAPI_Writer stlwriter;

    stlwriter.ASCIIMode() = (ext==".stl");
    //stlwriter.RelativeMode()=false;
    //stlwriter.SetDeflection(maxdefl);
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<9))
    stlwriter.SetCoefficient(5e-5);
#endif
    stlwriter.Write(shape(), filename.string().c_str());
  }
  else
  {
    throw insight::Exception("Unknown export file format! (Extension "+ext+")");
  }
}

void Feature::exportSTL(const boost::filesystem::path& filename, double abstol, bool binary) const
{
  BRepBuilderAPI_Copy aCopy( shape(), Standard_False );
  TopoDS_Shape os=aCopy.Shape();

  StlAPI_Writer stlwriter;
  stlwriter.ASCIIMode() = !binary; //false;

#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<9))
  stlwriter.RelativeMode()=false;
  stlwriter.SetDeflection(abstol);
#else
  BRepTools::Clean( os );
//  ShapeFix_ShapeTolerance sf;
//  sf.SetTolerance(os, abstol);
  BRepMesh_IncrementalMesh binc(os, abstol);
#endif

  stlwriter.Write(os, filename.string().c_str());
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
  
  for (const FeatureID& fi: fs.data())
  {
    
    TopoDS_Edge e=fs.model()->edge(fi);
    if (!BRep_Tool::Degenerated(e))
    {
      BRepAdaptor_Curve ac(e);
      GCPnts_QuasiUniformDeflection qud(ac, abstol);

      if (!qud.IsDone())
	throw insight::Exception("Discretization of curves into eMesh failed!");

      size_t iofs=points.size();

      if (qud.NbPoints()<2)
        throw insight::Exception("Edge discretizer returned less then 2 points !");

      for (int j=2; j<=qud.NbPoints(); j++)
      {
        arma::mat lp=insight::Vector(ac.Value(qud.Parameter(j-1)));
        if (j==2) points.push_back(lp);

        arma::mat p=insight::Vector(ac.Value(qud.Parameter(j)));

        double clen=norm(p-lp, 2);
        double rlen=clen;
        while (rlen>maxlen)
        {
          arma::mat pi = lp + maxlen*(p-lp)/arma::norm(p-lp,2);
          lp=pi;
          points.push_back(pi); // insert intermediate point
          rlen -= maxlen;
        }
        points.push_back(p);

      }
      
      for (size_t i=1; i<points.size()-iofs; i++)
      {
	int from=iofs+i-1;
	int to=iofs+i;
//	if (splits.find(to)==splits.end())
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
  for (const arma::mat& p: points)
  {
    f<<OFDictData::to_OF(p)<<endl;
  }
  f<<")"<<endl;

  f<<edges.size()<<endl
   <<"("<<endl;
  for (const Edge& e: edges)
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
    throw insight::cad::CADException(shared_from_this(), "Internal error: recursion during build!");

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
        std::shared_ptr<TopTools_ListOfShape> xsecs(new TopTools_ListOfShape);
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
        dispshape=dispshapes;
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
  if (isSingleEdge())
  {
      return BRepBuilderAPI_MakeWire(asSingleEdge()).Wire();
  }
  else if (isSingleWire())
  {
    return TopoDS::Wire(shape());
  }
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
    for (const RefValuesList::value_type& v: m1.getDatumScalars())
    {
        if (excludes.find(v.first)==excludes.end())
        {
            if (refvalues_.find(prefix+v.first)!=refvalues_.end())
                throw insight::Exception("datum value "+prefix+v.first+" already present!");
            refvalues_[prefix+v.first]=v.second;
        }
    }
    for (const RefPointsList::value_type& p: m1.getDatumPoints())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refpoints_.find(prefix+p.first)!=refpoints_.end())
                throw insight::Exception("datum point "+prefix+p.first+" already present!");
            refpoints_[prefix+p.first]=p.second;
        }
    }
    for (const RefVectorsList::value_type& p: m1.getDatumVectors())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refvectors_.find(prefix+p.first)!=refvectors_.end())
                throw insight::Exception("datum vector "+prefix+p.first+" already present!");
            refvectors_[prefix+p.first]=p.second;
        }
    }
    for (const SubfeatureMap::value_type& sf: m1.providedSubshapes())
    {
        if (excludes.find(sf.first)==excludes.end())
        {
            if (providedSubshapes_.find(prefix+sf.first)!=providedSubshapes_.end())
                throw insight::Exception("subshape "+prefix+sf.first+" already present!");
            providedSubshapes_[prefix+sf.first]=sf.second;
        }
    }
    for (const DatumPtrMap::value_type& df: m1.providedDatums())
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
    for (const RefValuesList::value_type& v: m1.getDatumScalars())
    {
        if (excludes.find(v.first)==excludes.end())
        {
            if (refvalues_.find(prefix+v.first)!=refvalues_.end())
                throw insight::Exception("datum value "+prefix+v.first+" already present!");
            refvalues_[prefix+v.first]=v.second;
        }
    }
    for (const RefPointsList::value_type& p: m1.getDatumPoints())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refpoints_.find(prefix+p.first)!=refpoints_.end())
                throw insight::Exception("datum point "+prefix+p.first+" already present!");
            refpoints_[prefix+p.first]=vec3(to_Pnt(p.second).Transformed(trsf));
        }
    }
    for (const RefVectorsList::value_type& p: m1.getDatumVectors())
    {
        if (excludes.find(p.first)==excludes.end())
        {
            if (refvectors_.find(prefix+p.first)!=refvectors_.end())
                throw insight::Exception("datum vector "+prefix+p.first+" already present!");
            refvectors_[prefix+p.first]=vec3(to_Vec(p.second).Transformed(trsf));
        }
    }
    for (const SubfeatureMap::value_type& sf: m1.providedSubshapes())
    {
        if (excludes.find(sf.first)==excludes.end())
        {
            if (providedSubshapes_.find(prefix+sf.first)!=providedSubshapes_.end())
                throw insight::Exception("subshape "+prefix+sf.first+" already present!");
            providedSubshapes_[prefix+sf.first]=Transform::create(sf.second, trsf);
        }
    }
    for (const DatumPtrMap::value_type& df: m1.providedDatums())
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
    for (const RefValuesList::value_type& i: refvalues_)
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
    for (const RefPointsList::value_type& i: refpoints_)
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
    for (const RefVectorsList::value_type& i: refvectors_)
    {
      av<<" "<<i.first;
    }
    throw insight::Exception("the feature does not define a reference vector named \""+name+"\". Available:"+av.str());
    return arma::mat();
  }
}

#define GMSH_NUMBERING_V2
#undef GMSH_DEBUG

void Feature::nameFeatures()
{
  // Don't call "shape()" here!
    idx_.reset(new SubshapeNumbering(shape_));
}

void Feature::extractReferenceFeatures()
{
  ///////////////////////////////////////////////////////////////////////////////
  /////////////// save reference points
  auto allptidx=allVerticesSet();
  for (int i: allptidx)
  {
    refpoints_[ str(format("v%d")%i) ] = vertexLocation(i);
  }
}

const TopoDS_Face& Feature::face(FeatureID i) const
{
    checkForBuildDuringAccess();
//    insight::assertion(
//                i>=1 && i<=idx_->_fmap.Extent(),
//                str(format("face ID %d out of range (%d...%d)")
//                    % i % 1 % idx_->_fmap.Extent()) );
//    return TopoDS::Face(idx_->_fmap.FindKey(i));
    return idx_->faceByTag(i);
}

const TopoDS_Edge& Feature::edge(FeatureID i) const
{
    checkForBuildDuringAccess();
//    insight::assertion(
//                i>=1 && i<=idx_->_emap.Extent(),
//                str(format("edge ID %d out of range (%d...%d)")
//                    % i % 1 % idx_->_emap.Extent()) );
//    return TopoDS::Edge(idx_->_emap.FindKey(i));
    return idx_->edgeByTag(i);
}

const TopoDS_Vertex& Feature::vertex(FeatureID i) const
{
    checkForBuildDuringAccess();
//    insight::assertion(
//                i>=1 && i<=idx_->_vmap.Extent(),
//                str(format("vertex ID %d out of range (%d...%d)")
//                    % i % 1 % idx_->_vmap.Extent()) );
//    return TopoDS::Vertex(idx_->_vmap.FindKey(i));
    return idx_->vertexByTag(i);
}

const TopoDS_Solid& Feature::subsolid(FeatureID i) const
{
    checkForBuildDuringAccess();
//    insight::assertion(
//                i>=1 && i<=idx_->_somap.Extent(),
//                str(format("solid ID %d out of range (%d...%d)")
//                    % i % 1 % idx_->_somap.Extent()) );
//    return TopoDS::Solid(idx_->_somap.FindKey(i));
    return idx_->solidByTag(i);
}


FeatureID Feature::solidID(const TopoDS_Shape& f) const
{
    checkForBuildDuringAccess();
//    int i=idx_->_somap.FindIndex(f);
//    if (i==0)
//        throw insight::Exception("requested solid not indexed!");
//    return i;
    return idx_->tagOfSolid(TopoDS::Solid(f));
}

FeatureID Feature::faceID(const TopoDS_Shape& f) const
{
    checkForBuildDuringAccess();
//    int i=idx_->_fmap.FindIndex(f);
//    if (i==0)
//        throw insight::Exception("requested face not indexed!");
//    return i;
    return idx_->tagOfFace(TopoDS::Face(f));
}

FeatureID Feature::edgeID(const TopoDS_Shape& e) const
{
    checkForBuildDuringAccess();
//    int i=idx_->_emap.FindIndex(e);
//    if (i==0)
//        throw insight::Exception("requested edge not indexed!");
//    return i;
    return idx_->tagOfEdge(TopoDS::Edge(e));
}

FeatureID Feature::vertexID(const TopoDS_Shape& v) const
{
    checkForBuildDuringAccess();
//    int i=idx_->_vmap.FindIndex(v);
//    if (i==0)
//        throw insight::Exception("requested vertex not indexed!");
//    return i;
    return idx_->tagOfVertex(TopoDS::Vertex(v));
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
    f<<buf.size()<<endl;
    f<<buf<<endl;
  }


//   RefValuesList refvalues_;
  f<<refvalues_.size()<<endl;
  for (const RefValuesList::value_type& i: refvalues_)
  {
    f<<i.first<<endl;
    f<<i.second<<endl;
  }
//   RefPointsList refpoints_;
  f<<refpoints_.size()<<endl;
  for (const RefPointsList::value_type& i: refpoints_)
  {
    f<<i.first<<endl;
    f<<i.second(0)<<" "<<i.second(1)<<" "<<i.second(2)<<endl;
  }
//   RefVectorsList refvectors_;
  f<<refvectors_.size()<<endl;
  for (const RefVectorsList::value_type& i: refvectors_)
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

/*
CREATE_FUNCTION();
static
 override
addToStaticFunctionTable(Feature, , insertrule);
addToStaticFunctionTable(Feature, , ruleDocumentation);
std::make_shared<parser::ISCADParser::ModelstepRule>(
*/

#warning circumvent
//addToStaticFunctionTable(Feature, Feature, insertrule);

void Feature::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
      (
          "asModel",
          std::make_shared<parser::ISCADParser::ModelstepRule>(

              ( '(' > ( ruleset.r_vertexFeaturesExpression | ruleset.r_edgeFeaturesExpression | ruleset.r_faceFeaturesExpression | ruleset.r_solidFeaturesExpression ) >> ')' )
                  [ qi::_val = phx::bind(Feature::create<FeatureSetPtr>, qi::_1) ]

              )
          );
}

#warning circumvent
//addToStaticFunctionTable(Feature, Feature, ruleDocumentation);

FeatureCmdInfoList Feature::ruleDocumentation()
{
  return {
    FeatureCmdInfo
    (
        "asModel",
        "( <verticesSelection>|<edgesSelection>|<facesSelection>|<solidSelection> )",
        "Creates a new feature from selected entities of an existing feature."
    )
  };
}

string Feature::generateScriptCommand() const
{
    return std::string();
}


bool Feature::TopologicalProperties::onlyEdges() const
{
    return nEdges>0 && nFaces==0;
}


Feature::TopologicalProperties Feature::topologicalProperties() const
{
    ShapeAnalysis_ShapeContents sas;
    sas.Perform(shape());
    return {
        sas.NbVertices(), sas.NbEdges(),
        sas.NbFaces(), sas.NbShells(),
        sas.NbSolids() };
}




Mass_CoG_Inertia compoundProps(const std::vector<std::shared_ptr<Feature> >& feats, double density_ovr, double aw_ovr)
{
  double m=0.0;
  arma::mat cog=vec3(0,0,0);
  
  double mcs[feats.size()];
  arma::mat cogs[feats.size()];
  
  int i=-1;
  for (const FeaturePtr& f: feats)
  {
      i++;
    mcs[i]=f->mass(density_ovr, aw_ovr);
    cogs[i]=f->modelCoG(density_ovr);

    m += mcs[i];
    cog += cogs[i]*mcs[i];
  }
  cog/=m;
  
  arma::mat inertia=arma::zeros(3,3);
  i=-1;
  for (const FeaturePtr& f: feats)
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



}


std::shared_ptr<PathParameter> make_filepath(
        cad::FeaturePtr ft,
        const boost::filesystem::path &originalFilePath)
{
    TemporaryFile tf(
                originalFilePath.filename().stem().string()
                +"-%%%%%%"
                +originalFilePath.extension().string()
                );
    ft->saveAs( tf.path() );
    return std::make_shared<PathParameter>(
          FileContainer(originalFilePath, tf),
          "temporary file path" );
}



}
