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

#undef BOOST_SPIRIT_DEBUG

#include "solidmodel.h"
#include "dxfwriter.h"
#include "datum.h"
#include "sketch.h"
#include "evaluation.h"
#include "freeship_interface.h"

#include "base/analysis.h"
#include "parser.h"
#include "boost/locale.hpp"
#include "base/boost_include.h"
#include "boost/make_shared.hpp"

#include "solidmodels.h"
#include "meshing.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

// phx::at shall return value reference instead of key/value pair
namespace boost { namespace phoenix { namespace stl {
    template <typename This, typename Key, typename Value, typename Compare, typename Allocator, typename Index>
        struct at_impl::result<This(std::map<Key,Value,Compare,Allocator>&, Index)>
        {
            typedef Value & type;
        };
    template <typename This, typename Key, typename Value, typename Compare, typename Allocator, typename Index>
        struct at_impl::result<This(std::map<Key,Value,Compare,Allocator> const&, Index)>
        {
            typedef Value const& type;
        };
}}}


namespace insight {
namespace cad {
  
namespace parser {  

Model::Model(const ModelSymbols& syms)
{
  addScalarSymbol( "M_PI", 	M_PI);
  addScalarSymbol( "deg", 	M_PI/180.);
  addVectorSymbol( "O", 	vec3(0,0,0) );
  addVectorSymbol( "EX", 	vec3(1,0,0) );
  addVectorSymbol( "EY", 	vec3(0,1,0) );
  addVectorSymbol( "EZ", 	vec3(0,0,1) );
  addDatumSymbol ( "XY", 	DatumPtr(new DatumPlane(vec3(0,0,0), vec3(0,0,1), vec3(0,1,0))) );
  addDatumSymbol ( "XZ", 	DatumPtr(new DatumPlane(vec3(0,0,0), vec3(0,1,0), vec3(1,0,0))) );
  addDatumSymbol ( "YZ", 	DatumPtr(new DatumPlane(vec3(0,0,0), vec3(1,0,0), vec3(0,1,0))) );
  
  BOOST_FOREACH(const ModelSymbols::value_type& s, syms)
  {
    const std::string& name=boost::fusion::at_c<0>(s);
    cout<<"Insert symbol:"<<name<<endl;
    if ( const scalar* sv = boost::get<scalar>( &boost::fusion::at_c<1>(s) ) )
    {
        addScalarSymbol(name, *sv);
	cout<<(*sv)<<endl;
    }
    else if ( const vector* vv = boost::get<vector>( &boost::fusion::at_c<1>(s) ) )
    {
        addVectorSymbol(name, *vv);
	cout<<(*vv)<<endl;
    }
  }
}

mapkey_parser::mapkey_parser<scalar> Model::scalarSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<scalar>(scalarSymbols_); 
}
mapkey_parser::mapkey_parser<vector> Model::vectorSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<vector>(vectorSymbols_); 
}
mapkey_parser::mapkey_parser<datum> Model::datumSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<datum>(datumSymbols_); 
}
mapkey_parser::mapkey_parser<solidmodel> Model::modelstepSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<solidmodel>(modelstepSymbols_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::vertexFeatureSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(vertexFeatureSymbols_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::edgeFeatureSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(edgeFeatureSymbols_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::faceFeatureSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(faceFeatureSymbols_); 
}
mapkey_parser::mapkey_parser<FeatureSetPtr> Model::solidFeatureSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<FeatureSetPtr>(solidFeatureSymbols_); 
}
mapkey_parser::mapkey_parser<Model::Ptr> Model::modelSymbolNames() const 
{
  return mapkey_parser::mapkey_parser<Model::Ptr>(modelSymbols_); 
}
// solidmodel import(const boost::filesystem::path& filepath)
// {
//   cout << "reading model "<<filepath<<endl;
//   return solidmodel(new SolidModel(filepath));
// }

arma::mat Model::modelCoG()
{
  double mtot=0.0;
  arma::mat cog=vec3(0,0,0);
  BOOST_FOREACH(const std::string cn, components_)
  {
    const SolidModel& m = *(modelstepSymbols_.find(cn)->second);
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

arma::mat Model::modelBndBox(double deflection) const
{

}


double dot(const vector& v1, const vector& v2)
{
  return arma::as_scalar(arma::dot(v1,v2));
}

vector rot(const vector& v, const scalar& a, const vector& ax)
{
  return rotMatrix(a, ax)*v;
}
BOOST_PHOENIX_ADAPT_FUNCTION(vector, rot_, rot, 3);


FeatureSetPtr queryVertices(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.query_vertices(filterexpr, of).clone());
}

FeatureSetPtr queryAllVertices(const SolidModel& m)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.allVertices().clone());
}

FeatureSetPtr queryVerticesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(fs->model().query_vertices_subset(*fs, filterexpr, of).clone());
}

FeatureSetPtr queryEdges(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.query_edges(filterexpr, of).clone());
}

FeatureSetPtr queryAllEdges(const SolidModel& m)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.allEdges().clone());
}

FeatureSetPtr queryEdgesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(fs->model().query_edges_subset(*fs, filterexpr, of).clone());
}

FeatureSetPtr queryFaces(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.query_faces(filterexpr, of).clone());
}

FeatureSetPtr queryAllFaces(const SolidModel& m)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.allFaces().clone());
}

FeatureSetPtr queryFacesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(fs->model().query_faces_subset(*fs, filterexpr, of).clone());
}

FeatureSetPtr querySolids(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.query_solids(filterexpr, of).clone());
}

FeatureSetPtr queryAllSolids(const SolidModel& m)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(m.allSolids().clone());
}

FeatureSetPtr querySolidsSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
{
  using namespace std;
  using namespace insight::cad;
  return FeatureSetPtr(fs->model().query_solids_subset(*fs, filterexpr, of).clone());
}
void writeViews(const boost::filesystem::path& file, const solidmodel& model, const std::vector<viewdef>& viewdefs)
{
  SolidModel::Views views;
  BOOST_FOREACH(const viewdef& vd, viewdefs)
  {
    bool sec=boost::get<3>(vd);
    cout<<"is_section="<<sec<<endl;
    views[boost::get<0>(vd)]=model->createView
    (
      boost::get<1>(vd),
      boost::get<2>(vd),
      sec
    );
  }
  
  {
    DXFWriter::writeViews(file, views);
  }
}

boost::filesystem::path sharedModelFilePath(const std::string& name)
{
  std::vector<boost::filesystem::path> paths;
  const char* e=getenv("ISCAD_MODEL_PATH");
  if (e)
  {
    boost::split(paths, e, boost::is_any_of(":"));
  }
  {
    insight::SharedPathList spl;
    BOOST_FOREACH(const path& p, spl)
     paths.push_back(p/"iscad-library");
  }
  paths.push_back(".");
  
  BOOST_FOREACH(const boost::filesystem::path& ps, paths)
  {
    path p(ps/name); 
    if (exists(p))
    {
      return p;
    }
  }

  throw insight::Exception("Shared model file "+name+" not found.");
  return boost::filesystem::path();
}

Model::Ptr loadModel(const std::string& name, const ModelSymbols& syms)
{
  Model::Ptr model(new Model(syms));
  if (parseISCADModelFile(sharedModelFilePath(name+".iscad"), model))
  {
    cout<<"Successfully parsed model "<<name<<endl;
    return model;
  }
  cout<<"Failed to parse model "<<name<<endl;
  return Model::Ptr();
}

double lookupTable(const std::string& name, const std::string& keycol, double keyval, const std::string& depcol)
{
  std::ifstream f( (sharedModelFilePath(name+".csv")).c_str() );
  std::string line;
  std::vector<std::string> cols;
  getline(f, line);
  boost::split(cols, line, boost::is_any_of(";"));
  int ik=find(cols.begin(), cols.end(), keycol) - cols.begin();
  int id=find(cols.begin(), cols.end(), depcol) - cols.begin();
  while (!f.eof())
  {
    getline(f, line);
    boost::split(cols, line, boost::is_any_of(";"));
    double kv=lexical_cast<double>(cols[ik]);
    if (fabs(kv-keyval)<1e-6)
    {
      double dv=lexical_cast<double>(cols[id]);
      return dv;
    }
  }
  throw insight::Exception(
      "Table lookup of value "+lexical_cast<std::string>(keyval)+
      " in column "+keycol+" failed!");
  return 0.0;
}
BOOST_PHOENIX_ADAPT_FUNCTION(double, lookupTable_, lookupTable, 4);

template<class T>
T lookupMap(const std::map<std::string, T>& map, const std::string& key)
{
  typedef std::map<std::string, T > Map;
  typename Map::const_iterator i=map.find(key);
//   cout<<"lookup >"<<key<<"< in "<<map.size()<<endl;
  if (i!=map.end())
  {
//     cout<<"got: "<<i->second<<endl;
    return T(i->second);
  }
  else
    return T();
}


using namespace qi;
using namespace phx;
using namespace insight::cad;

template<class T>
void addSymbolIfNotPresent(qi::symbols<char, T>& table, 
			   std::string const& name, T const& value)
{
  const T* ptr=table.find(name);
  if (!ptr)
  {
    table.add(name, value);
    cout<<"Default value for symbol "<<name<<" ("<<value<<") was inserted."<<endl;
  }
  else
  {
    cout<<"Symbol "<<name<<" was present."<<endl;
  }
}

skip_grammar::skip_grammar()
: skip_grammar::base_type(skip, "PL/0")
{
    skip
	=   boost::spirit::ascii::space
	| repo::confix("/*", "*/")[*(qi::char_ - "*/")]
	| repo::confix("//", qi::eol)[*(qi::char_ - qi::eol)]
	| repo::confix("#", qi::eol)[*(qi::char_ - qi::eol)]
	;
}


typedef boost::fusion::vector3<std::string, FeatureSetPtr, boost::optional<double> > GroupDesc;
typedef std::vector<GroupDesc> GroupsDesc;

typedef boost::fusion::vector2<std::string, arma::mat> NamedVertex;
typedef std::vector<NamedVertex> NamedVertices;

void runGmsh
(
  const boost::filesystem::path& outpath,
  const SolidModel& model,
  const std::string& volname,
  std::vector<double> L,
  bool quad,
  const GroupsDesc& vertexGroups,
  const GroupsDesc& edgeGroups,
  const GroupsDesc& faceGroups,
  const NamedVertices& namedVertices/*,
  bool keeptmpdir=false*/
)
{
  GmshCase c(model, L[0], L[1]);
  if (!quad) c.setLinear();
  BOOST_FOREACH(const GroupDesc& gd, vertexGroups)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameVertices(gname, *gfs);
  }
  BOOST_FOREACH(const GroupDesc& gd, edgeGroups)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameEdges(gname, *gfs);
  }
  BOOST_FOREACH(const GroupDesc& gd, faceGroups)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameFaces(gname, *gfs);
  }
  BOOST_FOREACH(const NamedVertex& gd, namedVertices)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const arma::mat& loc=boost::fusion::at_c<1>(gd);
    c.addSingleNamedVertex(gname, loc);
  }
  
  BOOST_FOREACH(const GroupDesc& gd, vertexGroups)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<double> gs=boost::fusion::at_c<2>(gd))
    {
      cout<<"set vertex "<<gname<<" to L="<<*gs<<endl;
      c.setVertexLen(gname, *gs);
    }
  }
  BOOST_FOREACH(const GroupDesc& gd, edgeGroups)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<double> gs=boost::fusion::at_c<2>(gd))
    {
      c.setEdgeLen(gname, *gs);
    }
  }
  BOOST_FOREACH(const GroupDesc& gd, faceGroups)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<double> gs=boost::fusion::at_c<2>(gd))
    {
      c.setFaceEdgeLen(gname, *gs);
    }
  }
  c.doMeshing(volname, outpath/*, true*/);
}

double getVectorComponent(const arma::mat& v, int c)
{
  return v(c);
}

arma::mat getModelCoG(const SolidModel& m)
{
  return m.modelCoG();
}

// template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
ISCADParser::ISCADParser(Model::Ptr model)
: ISCADParser::base_type(r_model),
  model_(model)
{
  
    r_model =  *( r_assignment | r_modelstep | r_solidmodel_propertyAssignment /*| r_loadmodel*/ ) 
	      >> -( lit("@post")  >> *r_postproc);
    
    r_assignment = 
      ( r_identifier >> '=' >> lit("loadmodel") >> '(' >> r_identifier >> 
      *(',' >> (r_identifier >> '=' >> (r_scalarExpression|r_vectorExpression) ) ) >> ')' >> ';' )
	[ phx::bind(&Model::addModelSymbol, model_, qi::_1, loadModel_(qi::_2, qi::_3)) ]
      |
      ( r_identifier >> '='  >> r_scalarExpression >> ';') 
	[ phx::bind(&Model::addScalarSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> lit("?=")  >> r_scalarExpression >> ';') 
	[ phx::bind(&Model::addScalarSymbolIfNotPresent, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_vectorExpression >> ';') 
	[ phx::bind(&Model::addVectorSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> lit("?=")  >> r_vectorExpression >> ';') 
	[ phx::bind(&Model::addVectorSymbolIfNotPresent, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_vertexFeaturesExpression >> ';') 
	[ phx::bind(&Model::addVertexFeatureSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_edgeFeaturesExpression >> ';') 
	[ phx::bind(&Model::addEdgeFeatureSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_faceFeaturesExpression >> ';') 
	[ phx::bind(&Model::addFaceFeatureSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_solidFeaturesExpression >> ';') 
	[ phx::bind(&Model::addSolidFeatureSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_datumExpression >> ';') 
	[ phx::bind(&Model::addDatumSymbol, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '=' >> r_solidmodel_expression > ';' ) 
        [ phx::bind(&Model::addModelstepSymbol, model_, qi::_1, qi::_2) ]
      ;
      
/** @addtogroup cad_parser
  * @{
  * @section postproc Postprocessing statements
  * 
  * DXF(<filename>) << <SolidModel>
  * 
  *         <viewname> (<viewon>, <viewfrom> [, section]) 
  * 
  *        [<viewname> ...];
  * 
  * saveAs(<filename>) << <SolidModel>;
  * @}
  */

    r_postproc =
      ( lit("DXF") >> '(' >> r_path >> ')' >> lit("<<") >> r_solidmodel_expression >> *(r_viewDef) >> ';' ) 
	[ writeViews_(qi::_1, qi::_2, qi::_3) ]
      |
      ( lit("saveAs") >> '(' >> r_path >> ')' >> lit("<<") >> r_solidmodel_expression >> ';' ) 
	[ phx::bind(&SolidModel::saveAs, *qi::_2, qi::_1) ]
      |
      ( lit("gmsh") >> '(' >> r_path >> ')' >> lit("<<") 
        >> r_solidmodel_expression >> lit("as") >> r_identifier
        >> ( lit("L") >> '=' >> '(' >> repeat(2)[qi::double_] ) >> ')'
	>> ( ( lit("linear") >> attr(false) ) | attr(true) )
	>> lit("vertexGroups") >> '(' >> *( ( r_identifier >> '=' >> r_vertexFeaturesExpression >> -( '@' > double_ ) ) ) >> ')'
	>> lit("edgeGroups") >> '(' >> *( ( r_identifier >> '=' >> r_edgeFeaturesExpression >> -( '@' > double_ ) )  ) >> ')'
	>> lit("faceGroups") >> '(' >> *( ( r_identifier >> '=' >> r_faceFeaturesExpression >> -( '@' > double_ ) )  ) >> ')'
	>> ( lit("vertices") >> '(' >> *( r_identifier >> '=' >> r_vectorExpression ) >> ')' | attr(NamedVertices()) )
//         >> ( (lit("keeptmpdir")>attr(true)) | attr(false) )
	>> ';' )
	[ phx::bind(&runGmsh, qi::_1, *qi::_2, qi::_3, 
		    qi::_4, qi::_5, 
		    qi::_6,
		    qi::_7, qi::_8, qi::_9
 		  ) ]
      |
      ( lit("exportSTL") >> '(' >> r_path >> ',' >> r_scalarExpression >> ')' >> lit("<<") >> r_solidmodel_expression >> ';' ) 
	[ phx::bind(&SolidModel::exportSTL, *qi::_3, qi::_1, qi::_2) ]
      |
      ( lit("exportEMesh") >> '(' >> r_path >> ',' >> r_scalarExpression >> ',' >> r_scalarExpression >> ')' >> lit("<<") >> r_edgeFeaturesExpression >> ';' ) 
	[ phx::bind(&SolidModel::exportEMesh, 
		    qi::_1, *qi::_4, qi::_2, qi::_3) ]
      |
      ( lit("exportFreeShipSurface") >> '(' >> r_path >> ')' >> lit("<<") >> r_solidmodel_expression >> ';' ) 
	[ phx::bind(&writeFreeShipSurface, *qi::_2, qi::_1) ]
      |
//       ( lit("writeHydrostatics") >> '(' >> r_path >> ')' >> lit("<<") >> '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' >> ';' ) 
// 	[ phx::bind(&writeHydrostaticReport, *qi::_2, qi::_3, qi::_4, qi::_1) ]
//       |
      ( lit("SolidProperties") >> '(' >> r_identifier >> ')' >> lit("<<") >> r_solidmodel_expression >> ';' )
	[ phx::bind(&Model::addEvaluationSymbol, model_, qi::_1, 
		    phx::construct<EvaluationPtr>(new_<SolidProperties>(*qi::_2))) 
	]
      |
      ( lit("Hydrostatics") >> '(' 
	  >> r_identifier >> ',' 
	  >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' 
	  >> r_vectorExpression >> ',' >> r_vectorExpression
	  >> ')' >> lit("<<") >> '(' >> r_solidmodel_expression >> ',' >> r_solidmodel_expression >> ')' >> ';' ) // (1) hull and (2) ship
	[ phx::bind(&Model::addEvaluationSymbol, model_, qi::_1, 
		    phx::construct<EvaluationPtr>(new_<Hydrostatics>(*qi::_6, *qi::_7, qi::_2, qi::_3, qi::_4, qi::_5))) 
	]
      |
      ( lit("show") >> '(' >> r_vectorExpression >> ',' >> r_identifier >> ')' >> ';' ) 
	[ phx::bind(&Model::addEvaluationSymbol, model_, qi::_2, 
		    phx::construct<EvaluationPtr>(new_<showPoint>(qi::_1, qi::_2))) 
	]
// 	[ phx::bind(&showPoint, *qi::_2, qi::_1) ]
      ;
      
    r_viewDef =
	(r_identifier >> '(' 
	  >> r_vectorExpression >> ',' 
	  >> r_vectorExpression
	  >> ( ( ',' >> lit("section") >> qi::attr(true) ) | attr(false) )
	  >> ')' 
	)
      ;
    
    r_modelstep  =  ( r_identifier >> ':' > r_solidmodel_expression > ';' ) 
      [ phx::bind(&Model::addComponent, model_, qi::_1, qi::_2) ]
	;
    
    
    r_solidmodel_expression =
      r_solidmodel_term [_val=qi::_1 ]
      >> *( '-' >> r_solidmodel_term [ _val = construct<solidmodel>(new_<BooleanSubtract>(*_val, *qi::_1)) ] )
      ;
    
    r_solidmodel_term =
      r_solidmodel_primary [_val=qi::_1 ]
      >> *( 
      ('|' >> r_solidmodel_primary [ _val = construct<solidmodel>(new_<BooleanUnion>(*_val, *qi::_1)) ] )
      |
      ('&' >> r_solidmodel_primary [ _val = construct<solidmodel>(new_<BooleanIntersection>(*_val, *qi::_1)) ] )
      )
      ;

    r_modelstepFunction %= omit [ modelstepFunctionRules[ qi::_a = qi::_1 ] ] > qi::lazy(*qi::_a);
    
    r_solidmodel_primary = 
      ( '(' >> r_solidmodel_expression [ _val = qi::_1] > ')' )
      |
      r_modelstepFunction [ _val = qi::_1 ]
      // try identifiers last, since exceptions are generated, if symbols don't exist
      | 
	r_solidmodel_subshape [ _val = qi::_1 ]
      | 
	r_submodel_modelstep [ _val = qi::_1 ]
      |
	qi::lexeme [ model_->modelstepSymbolNames() ] [ _val =  phx::bind(&Model::lookupModelstepSymbol, model_, qi::_1) ]
      ;
      
    r_submodel_modelstep =
      qi::lexeme[ model_->modelSymbolNames() ] [ _a =  phx::bind(&Model::lookupModelSymbol, model_, qi::_1) ]
	>> lit('.') > /*lazy(phx::val(phx::bind(&Model::modelstepSymbolNames, *_a)))*/ 
	r_identifier [ _val =  phx::bind(&Model::lookupModelstepSymbol, *_a, qi::_1) ]
	;
      
    r_solidmodel_subshape =
      qi::lexeme[ model_->modelstepSymbolNames() ] [ _a =  phx::bind(&Model::lookupModelstepSymbol, model_, qi::_1) ] 
	  >> lit('.') > 
	  lazy( phx::val(phx::bind(&SolidModel::providedSubshapes, *_a)) )
	    [ _val = qi::_1 ]
	  ;

    r_solidmodel_propertyAssignment =
      qi::lexeme[ model_->modelstepSymbolNames() ] [ _a =  phx::bind(&Model::lookupModelstepSymbol, model_, qi::_1) ] 
	  > lit("->") >
	   (
	     ( lit("CoG") > '=' > r_vectorExpression ) [ lazy( phx::bind(&SolidModel::setCoGExplicitly, *_a, qi::_1) ) ]
	     |
	     ( lit("mass") > '=' > r_scalarExpression ) [ lazy( phx::bind(&SolidModel::setMassExplicitly, *_a, qi::_1) ) ]
	     |
	     ( lit("density") > '=' > r_scalarExpression ) [ lazy( phx::bind(&SolidModel::setDensity, *_a, qi::_1) ) ]
	     |
	     ( lit("areaWeight") > '=' > r_scalarExpression ) [ lazy( phx::bind(&SolidModel::setAreaWeight, *_a, qi::_1) ) ]
	   )
	  > ';'
	  ;

    r_vertexFeaturesExpression = 
	  qi::lexeme[model_->vertexFeatureSymbolNames()] 
	    [ _val =  phx::bind(&Model::lookupVertexFeatureSymbol, model_, qi::_1) ]
	  | (
	   ( lit("verticesFrom") >> r_solidmodel_expression 
	    >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = queryVertices_(*qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("verticesFrom") >> r_vertexFeaturesExpression >> lit("where") >> '(' >> r_string
	    >> *( ',' >> (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = queryVerticesSubset_(qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("allVerticesFrom") >> r_solidmodel_expression )
	    [ _val = queryAllVertices_(*qi::_1) ]
	  )
      ;

    r_edgeFeaturesExpression = 
	  qi::lexeme[model_->edgeFeatureSymbolNames()] [ _val =  phx::bind(&Model::lookupEdgeFeatureSymbol, model_, qi::_1) ]
	  | (
	   ( lit("edgesFrom") >> r_solidmodel_expression >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_edgeFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = queryEdges_(*qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("edgesFrom") >> r_edgeFeaturesExpression >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_edgeFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = queryEdgesSubset_(qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("allEdgesFrom") >> r_solidmodel_expression )
	    [ _val = queryAllEdges_(*qi::_1) ]
	  )
      ;

    r_faceFeaturesExpression = 
	  qi::lexeme[model_->faceFeatureSymbolNames()] 
	    [ _val =  phx::bind(&Model::lookupFaceFeatureSymbol, model_, qi::_1) ]
	  | (
	   ( lit("facesFrom") >> r_solidmodel_expression >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = queryFaces_(*qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("facesFrom") >> r_faceFeaturesExpression >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = queryFacesSubset_(qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("allFacesFrom") >> r_solidmodel_expression )
	    [ _val = queryAllFaces_(*qi::_1) ]
	  )
	;

    r_solidFeaturesExpression = 
	  qi::lexeme[model_->solidFeatureSymbolNames()] 
	    [ _val =  phx::bind(&Model::lookupSolidFeatureSymbol, model_, qi::_1) ]
	  | (
	   ( lit("solidsFrom") >> r_solidmodel_expression >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = querySolids_(*qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("solidsFrom") >> r_faceFeaturesExpression >> lit("where") >> '(' >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) ) >> ')' )
	    [ _val = querySolidsSubset_(qi::_1, qi::_2, qi::_3) ]
	   |
	   ( lit("allSolidsFrom") >> r_solidmodel_expression )
	    [ _val = queryAllSolids_(*qi::_1) ]
	  )
	;

      
    r_datumExpression = 
	  qi::lexeme[model_->datumSymbolNames()] [ _val =  phx::bind(&Model::lookupDatumSymbol, model_, qi::_1) ]
	  |
	  ( lit("Plane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2)) ]
	  |
	  ( lit("SPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2, qi::_3)) ]
      ;
      
    r_path = as_string[ 
			lexeme [ "\"" >> *~char_("\"") >> "\"" ] 
		      ];
    r_string = as_string[ 
			lexeme [ "\'" >> *~char_("\'") >> "\'" ] 
		      ];
		      
    r_scalarExpression = 
      r_scalar_term [_val =qi::_1]  >> *(
	( '+' >> r_scalar_term [_val+=qi::_1] )
      | ( '-' >> r_scalar_term [_val-=qi::_1] )
      )
      ;
    
    r_scalar_term =
    (
      r_scalar_primary [_val=qi::_1] >> *(
	( '*' >> r_scalar_primary [ _val*=qi::_1 ] )
      | ( '/' >> r_scalar_primary [ _val/=qi::_1 ] )
      )
    ) | (
      r_vector_primary >> '&' >> r_vector_primary
    ) [_val = dot_(qi::_1, qi::_2) ]
      ;
      
    r_scalar_primary =
// 	  lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = qi::_1 ]
      qi::lexeme[model_->scalarSymbolNames()]
	[ _val = phx::bind(&Model::lookupScalarSymbol, model_, qi::_1) ]
      | double_ [ _val = qi::_1 ]
      | ( lit("pow") >> '(' >> r_scalarExpression >> ',' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::pow, qi::_1, qi::_2) ]
      | ( lit("sqrt") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::sqrt, qi::_1) ]
      | ( lit("sin") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::sin, qi::_1) ]
      | ( lit("cos") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::cos, qi::_1) ]
      | ( lit("tan") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::tan, qi::_1) ]
      | ( lit("asin") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::asin, qi::_1) ]
      | ( lit("acos") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::acos, qi::_1) ]
      | ( lit("atan") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::atan, qi::_1) ]
      | ( lit("atan2") >> '(' >> r_scalarExpression >> ',' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::atan2, qi::_1, qi::_2) ]
      | ('(' >> r_scalarExpression >> ')') [_val=qi::_1]
      | ( lit("TableLookup") > '(' > r_identifier > ',' 
	    > r_identifier > ',' > r_scalarExpression > ',' > r_identifier > ')' )
	[ _val = lookupTable_(qi::_1, qi::_2, qi::_3, qi::_4) ]
      | ( r_vector_primary >> '.' >> 'x' ) [ _val = phx::bind(&getVectorComponent, qi::_1, 0) ]
      | ( r_vector_primary >> '.' >> 'y' ) [ _val = phx::bind(&getVectorComponent, qi::_1, 1) ]
      | ( r_vector_primary >> '.' >> 'z' ) [ _val = phx::bind(&getVectorComponent, qi::_1, 2) ]
      | ('-' >> r_scalar_primary) [_val=-qi::_1]
      ;


    r_vectorExpression =
      r_vector_term [_val =qi::_1]  >> *(
	( '+' >> r_vector_term [_val+=qi::_1] )
      | ( '-' >> r_vector_term [_val-=qi::_1] )
      )
      ;
    
    r_vector_term =
    (
      r_vector_primary [_val=qi::_1] >> *(
	( '*' >> r_scalar_term [ _val*=qi::_1 ] )
      | ( '/' >> r_scalar_term [ _val/=qi::_1 ] )
      | ( '^' >> r_vector_primary [ _val=cross_(_val, qi::_1) ] )
      )
    ) | (
      r_scalar_primary >> '*' >> r_vector_term
    ) [_val=qi::_1*qi::_2]
    ;
      
    r_vector_primary =
       ( lit("modelCoG") )
        [ _val = phx::bind(&Model::modelCoG, model_) ]
      |
       ( lit("rot") > '(' > r_vectorExpression > lit("by") > r_scalarExpression > ( (lit("around") > r_vectorExpression) | attr(vec3(0,0,1)) )> ')' )
        [ _val = rot_(qi::_1, qi::_2, qi::_3) ]
      |
       qi::lexeme[model_->vectorSymbolNames()] 
        [ _val =  phx::bind(&Model::lookupVectorSymbol, model_, qi::_1) ]
      |
      qi::lexeme[ model_->modelstepSymbolNames() ] [ _a =  phx::bind(&Model::lookupModelstepSymbol, model_, qi::_1) ] 
	  > lit("->") >
	   (
	     lit("CoG") [ lazy( _val = phx::bind(&getModelCoG, *_a)) ]
	   )
      |
       ( "[" >> r_scalarExpression >> "," >> r_scalarExpression >> "," >> r_scalarExpression >> "]" ) 
        [ _val = vec3_(qi::_1, qi::_2, qi::_3) ] 
      //| ( r_vectorExpression >> '\'') [ _val = trans_(qi::_1) ]
      |
       ( '(' >> r_vectorExpression >> ')' ) 
        [_val=qi::_1]
      |
       ( '-' >> r_vector_primary ) 
        [_val=-qi::_1]
      ;

    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
    
    for (SolidModel::FactoryTable::const_iterator i = SolidModel::factories_->begin();
	i != SolidModel::factories_->end(); i++)
    {
      SolidModelPtr sm(i->second->operator()(NoParameters()));
      sm->insertrule(*this);
    }
    
      
// 	BOOST_SPIRIT_DEBUG_NODE(r_path);
// 	BOOST_SPIRIT_DEBUG_NODE(r_identifier);
// 	BOOST_SPIRIT_DEBUG_NODE(r_assignment);
// 	BOOST_SPIRIT_DEBUG_NODE(r_postproc);
// 	BOOST_SPIRIT_DEBUG_NODE(r_viewDef);
// 	BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);
// 	BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
// 	BOOST_SPIRIT_DEBUG_NODE(r_scalarExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_vector_primary);
// 	BOOST_SPIRIT_DEBUG_NODE(r_vector_term);
// 	BOOST_SPIRIT_DEBUG_NODE(r_vectorExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_edgeFeaturesExpression);
// // 	BOOST_SPIRIT_DEBUG_NODE(r_edgeFilterExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_expression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_propertyAssignment);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_subshape);
// 	BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
// 	BOOST_SPIRIT_DEBUG_NODE(r_model);
	BOOST_SPIRIT_DEBUG_NODE(r_vertexFeaturesExpression);
    
    on_error<fail>(r_model, 
	    phx::ref(std::cout)
		<< "Error! Expecting "
		<< qi::_4
		<< " here: '"
		<< phx::construct<std::string>(qi::_3, qi::_2)
		<< "'\n"
	);
}



struct ModelStepsWriter
//: public std::map<std::string, T>
{
    void operator() (std::string s, SolidModelPtr ct);
};

// template <typename Parser, typename Iterator>
bool parseISCADModel(std::string::iterator first, std::string::iterator last, Model::Ptr& model)
{
  ISCADParser parser(model);
  skip_grammar/*<Iterator>*/ skip;
  
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  
//   ModelStepsWriter writer;
//   parser.model_.modelstepSymbols.for_each(writer);

  if (first != last) // fail if we did not get a full match
      return false;
  
//   model = parser.model_;
  
  return r;
}

}
using namespace parser;


bool parseISCADModelFile(const boost::filesystem::path& fn, parser::Model::Ptr& m)
{
  std::ifstream f(fn.c_str());
  return parseISCADModelStream(f, m);
}

void ModelStepsWriter::operator() (std::string s, SolidModelPtr ct)
{
  //std::string s(ws.begin(), ws.end());
  //cout<<s<<endl<<ct<<endl;
  //(*this)[s]=ct;
  if (s=="final")
  {
    ct->saveAs(s+".brep");
    //ct->createView(vec3(0, 0, 0), vec3(0, -1, 0), false);
  }
}
    
bool parseISCADModelStream(std::istream& in, parser::Model::Ptr& m, int* failloc)
{
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  //in.close();
  
  std::string::iterator orgbegin,
    first=contents_raw.begin(), 
    last=contents_raw.end();
    
  orgbegin=first;
  
  ISCADParser/*<std::string::iterator>*/ parser(m);
  skip_grammar/*<std::string::iterator>*/ skip;
  
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  
//   ModelStepsWriter writer;
//   parser.model_.modelstepSymbols.for_each(writer);

  if (first != last) // fail if we did not get a full match
  {
    if (failloc) *failloc=int(first-orgbegin);
    return false;
  }
  
//   m = parser.model_;
  
  return r;
}

}
}
