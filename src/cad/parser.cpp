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

#include "cadfeature.h"

#include "datum.h"
#include "sketch.h"
#include "cadpostprocactions.h"

#include "base/analysis.h"
#include "parser.h"
#include "boost/locale.hpp"
#include "base/boost_include.h"
#include "boost/make_shared.hpp"

#include "cadfeatures.h"
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


// double dot(const vector& v1, const vector& v2)
// {
//   return arma::as_scalar(arma::dot(v1,v2));
// }
// 
// vector rot(const vector& v, const scalar& a, const vector& ax)
// {
//   return rotMatrix(a, ax)*v;
// }
// BOOST_PHOENIX_ADAPT_FUNCTION(vector, rot_, rot, 3);
// 
// 
// FeatureSetPtr queryVertices(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.query_vertices(filterexpr, of).clone());
// }
// 
// FeatureSetPtr queryAllVertices(const SolidModel& m)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.allVertices().clone());
// }
// 
// FeatureSetPtr queryVerticesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(fs->model().query_vertices_subset(*fs, filterexpr, of).clone());
// }
// 
// FeatureSetPtr queryEdges(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.query_edges(filterexpr, of).clone());
// }
// 
// FeatureSetPtr queryAllEdges(const SolidModel& m)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.allEdges().clone());
// }
// 
// FeatureSetPtr queryEdgesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(fs->model().query_edges_subset(*fs, filterexpr, of).clone());
// }
// 
// FeatureSetPtr queryFaces(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.query_faces(filterexpr, of).clone());
// }
// 
// FeatureSetPtr queryAllFaces(const SolidModel& m)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.allFaces().clone());
// }
// 
// FeatureSetPtr queryFacesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(fs->model().query_faces_subset(*fs, filterexpr, of).clone());
// }
// 
// FeatureSetPtr querySolids(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.query_solids(filterexpr, of).clone());
// }
// 
// FeatureSetPtr queryAllSolids(const SolidModel& m)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(m.allSolids().clone());
// }
// 
// FeatureSetPtr querySolidsSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of)
// {
//   using namespace std;
//   using namespace insight::cad;
//   return FeatureSetPtr(fs->model().query_solids_subset(*fs, filterexpr, of).clone());
// }
// void writeViews(const boost::filesystem::path& file, const solidmodel& model, const std::vector<viewdef>& viewdefs)
// {
//   SolidModel::Views views;
//   BOOST_FOREACH(const viewdef& vd, viewdefs)
//   {
//     bool sec=boost::get<3>(vd);
//     cout<<"is_section="<<sec<<endl;
//     views[boost::get<0>(vd)]=model->createView
//     (
//       boost::get<1>(vd),
//       boost::get<2>(vd),
//       sec
//     );
//   }
//   
//   {
//     DXFWriter::writeViews(file, views);
//   }
// }

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

// Model::Ptr loadModel(const std::string& name, const ModelSymbols& syms)
// {
//   Model::Ptr model(new Model(syms));
//   if (parseISCADModelFile(sharedModelFilePath(name+".iscad"), model))
//   {
//     cout<<"Successfully parsed model "<<name<<endl;
//     return model;
//   }
//   cout<<"Failed to parse model "<<name<<endl;
//   return Model::Ptr();
// }

// double lookupTable(const std::string& name, const std::string& keycol, double keyval, const std::string& depcol)
// {
//   std::ifstream f( (sharedModelFilePath(name+".csv")).c_str() );
//   std::string line;
//   std::vector<std::string> cols;
//   getline(f, line);
//   boost::split(cols, line, boost::is_any_of(";"));
//   int ik=find(cols.begin(), cols.end(), keycol) - cols.begin();
//   int id=find(cols.begin(), cols.end(), depcol) - cols.begin();
//   while (!f.eof())
//   {
//     getline(f, line);
//     boost::split(cols, line, boost::is_any_of(";"));
//     double kv=lexical_cast<double>(cols[ik]);
//     if (fabs(kv-keyval)<1e-6)
//     {
//       double dv=lexical_cast<double>(cols[id]);
//       return dv;
//     }
//   }
//   throw insight::Exception(
//       "Table lookup of value "+lexical_cast<std::string>(keyval)+
//       " in column "+keycol+" failed!");
//   return 0.0;
// }
// BOOST_PHOENIX_ADAPT_FUNCTION(double, lookupTable_, lookupTable, 4);

// template<class T>
// T lookupMap(const std::map<std::string, T>& map, const std::string& key)
// {
//   typedef std::map<std::string, T > Map;
//   typename Map::const_iterator i=map.find(key);
// //   cout<<"lookup >"<<key<<"< in "<<map.size()<<endl;
//   if (i!=map.end())
//   {
// //     cout<<"got: "<<i->second<<endl;
//     return T(i->second);
//   }
//   else
//     return T();
// }


using namespace qi;
using namespace phx;
using namespace insight::cad;

// template<class T>
// void addSymbolIfNotPresent(qi::symbols<char, T>& table, 
// 			   std::string const& name, T const& value)
// {
//   const T* ptr=table.find(name);
//   if (!ptr)
//   {
//     table.add(name, value);
//     cout<<"Default value for symbol "<<name<<" ("<<value<<") was inserted."<<endl;
//   }
//   else
//   {
//     cout<<"Symbol "<<name<<" was present."<<endl;
//   }
// }

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



// void runGmsh
// (
//   const boost::filesystem::path& outpath,
//   const SolidModel& model,
//   const std::string& volname,
//   std::vector<double> L,
//   bool quad,
//   const GroupsDesc& vertexGroups,
//   const GroupsDesc& edgeGroups,
//   const GroupsDesc& faceGroups,
//   const NamedVertices& namedVertices/*,
//   bool keeptmpdir=false*/
// )
// {
//   GmshCase c(model, L[0], L[1]);
//   if (!quad) c.setLinear();
//   BOOST_FOREACH(const GroupDesc& gd, vertexGroups)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
//     c.nameVertices(gname, *gfs);
//   }
//   BOOST_FOREACH(const GroupDesc& gd, edgeGroups)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
//     c.nameEdges(gname, *gfs);
//   }
//   BOOST_FOREACH(const GroupDesc& gd, faceGroups)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
//     c.nameFaces(gname, *gfs);
//   }
//   BOOST_FOREACH(const NamedVertex& gd, namedVertices)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     const arma::mat& loc=boost::fusion::at_c<1>(gd);
//     c.addSingleNamedVertex(gname, loc);
//   }
//   
//   BOOST_FOREACH(const GroupDesc& gd, vertexGroups)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     if (boost::optional<double> gs=boost::fusion::at_c<2>(gd))
//     {
//       cout<<"set vertex "<<gname<<" to L="<<*gs<<endl;
//       c.setVertexLen(gname, *gs);
//     }
//   }
//   BOOST_FOREACH(const GroupDesc& gd, edgeGroups)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     if (boost::optional<double> gs=boost::fusion::at_c<2>(gd))
//     {
//       c.setEdgeLen(gname, *gs);
//     }
//   }
//   BOOST_FOREACH(const GroupDesc& gd, faceGroups)
//   {
//     const std::string& gname=boost::fusion::at_c<0>(gd);
//     if (boost::optional<double> gs=boost::fusion::at_c<2>(gd))
//     {
//       c.setFaceEdgeLen(gname, *gs);
//     }
//   }
//   c.doMeshing(volname, outpath/*, true*/);
// }

// double getVectorComponent(const arma::mat& v, int c)
// {
//   return v(c);
// }
// 
// arma::mat getModelCoG(const SolidModel& m)
// {
//   return m.modelCoG();
// }

// template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
ISCADParser::ISCADParser(Model* model)
: ISCADParser::base_type(r_model),
  model_(model)
{
  
    r_model =  
     *( 
       r_assignment 
       | 
       r_modelstep 
       | 
       r_solidmodel_propertyAssignment
     ) 
      >> 
     -( 
       lit("@post")  
       >> *r_postproc
      );
    
     
    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
    
    r_path = as_string[ 
			lexeme [ "\"" >> *~char_("\"") >> "\"" ] 
		      ];
    r_string = as_string[ 
			lexeme [ "\'" >> *~char_("\'") >> "\'" ] 
		      ];

		      
    r_assignment = 
//       ( r_identifier >> '=' >> lit("loadmodel") >> '(' >> r_identifier >> 
//       *(',' >> (r_identifier >> '=' >> (r_scalarExpression|r_vectorExpression) ) ) >> ')' >> ';' )
// 	[ phx::bind(&Model::addModel, model_, qi::_1, 
// 		    phx::construct<ModelPtr>(phx::new_<Model>(qi::_2, qi::_3))) ]
//       |
      ( r_identifier >> '='  >> r_scalarExpression >> ';') 
	[ phx::bind(&Model::addScalar, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> lit("?=")  >> r_scalarExpression >> ';') 
	[ phx::bind(&Model::addScalarIfNotPresent, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_vectorExpression >> ';') 
	[ phx::bind(&Model::addVector, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> lit("?=")  >> r_vectorExpression >> ';') 
	[ phx::bind(&Model::addVectorIfNotPresent, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_vertexFeaturesExpression >> ';') 
	[ phx::bind(&Model::addVertexFeature, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_edgeFeaturesExpression >> ';') 
	[ phx::bind(&Model::addEdgeFeature, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_faceFeaturesExpression >> ';') 
	[ phx::bind(&Model::addFaceFeature, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_solidFeaturesExpression >> ';') 
	[ phx::bind(&Model::addSolidFeature, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_datumExpression >> ';') 
	[ phx::bind(&Model::addDatum, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '=' >> r_solidmodel_expression > ';' ) 
        [ phx::bind(&Model::addModelstep, model_, qi::_1, qi::_2) ]
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
      ( lit("DXF") > '(' > r_path > ')' > lit("<<") > r_solidmodel_expression > *(r_viewDef) > ';' ) 
        [ phx::bind(&Model::addPostprocActionUnnamed, model_, 
		    phx::construct<PostprocActionPtr>(new_<DrawingExport>(qi::_1, qi::_2, qi::_3))) ]
      |
      ( lit("saveAs") > '(' > r_path > ')' > lit("<<") > r_solidmodel_expression > ';' ) 
        [ phx::bind(&Model::addPostprocActionUnnamed, model_, 
		    phx::construct<PostprocActionPtr>(new_<Export>(qi::_2, qi::_1))) ]
      |
      ( lit("exportSTL") > '(' > r_path > ',' > r_scalarExpression > ')' > lit("<<") > r_solidmodel_expression > ';' ) 
        [ phx::bind(&Model::addPostprocActionUnnamed, model_, 
		    phx::construct<PostprocActionPtr>(new_<Export>(qi::_3, qi::_1, qi::_2))) ]
      |
      ( lit("exportEMesh") > '(' > r_path > ',' > r_scalarExpression > ',' > r_scalarExpression > ')' 
          > lit("<<") > r_edgeFeaturesExpression > ';' ) 
        [ phx::bind(&Model::addPostprocActionUnnamed, model_, 
		    phx::construct<PostprocActionPtr>(new_<Export>(qi::_4, qi::_1, qi::_2, qi::_3))) ]
      |
//       ( lit("gmsh") > '(' > r_path > ')' > lit("<<") 
//         > r_solidmodel_expression >> lit("as") >> r_identifier
//         >> ( lit("L") >> '=' >> '(' >> repeat(2)[qi::double_] ) >> ')'
// 	>> ( ( lit("linear") >> attr(false) ) | attr(true) )
// 	>> lit("vertexGroups") >> '(' >> *( ( r_identifier >> '=' >> r_vertexFeaturesExpression >> -( '@' > r_scalarExpression ) ) ) >> ')'
// 	>> lit("edgeGroups") >> '(' >> *( ( r_identifier >> '=' >> r_edgeFeaturesExpression >> -( '@' > r_scalarExpression ) )  ) >> ')'
// 	>> lit("faceGroups") >> '(' >> *( ( r_identifier >> '=' >> r_faceFeaturesExpression >> -( '@' > r_scalarExpression ) )  ) >> ')'
// 	>> ( lit("vertices") >> '(' >> *( r_identifier >> '=' >> r_vectorExpression ) >> ')' | attr(NamedVertices()) )
// //         >> ( (lit("keeptmpdir")>attr(true)) | attr(false) )
// 	>> ';' )
//         [ phx::bind(&Model::addPostprocActionUnnamed, model_, 
// 		    phx::construct<PostprocActionPtr>(new_<Mesh>(
// 		    qi::_1, 
// 		    qi::_2, qi::_3, 
// 		    qi::_4, 
// 		    qi::_5, 
// 		    qi::_6,
// 		    qi::_7, 
// 		    qi::_8, 
// 		    qi::_9
// 		    ))) ]
//       |
      ( lit("SolidProperties") > '(' > r_identifier > ')' > lit("<<") > r_solidmodel_expression > ';' )
	[ phx::bind(&Model::addPostprocAction, model_, qi::_1, 
		    phx::construct<PostprocActionPtr>(new_<SolidProperties>(qi::_2))) 
	]
      |
      ( lit("Hydrostatics") > '(' 
	  > r_identifier > ',' 
	  > r_vectorExpression > ',' > r_vectorExpression > ',' 
	  > r_vectorExpression > ',' > r_vectorExpression
	  > ')' > lit("<<") > '(' > r_solidmodel_expression > ',' > r_solidmodel_expression > ')' > ';' ) // (1) hull and (2) ship
	[ phx::bind(&Model::addPostprocAction, model_, qi::_1, 
		    phx::construct<PostprocActionPtr>(new_<Hydrostatics>(qi::_6, qi::_7, qi::_2, qi::_3, qi::_4, qi::_5))) 
	]
      ;
      
    r_viewDef =
	(r_identifier >> '(' 
	  >> r_vectorExpression >> ',' 
	  >> r_vectorExpression
	  >> ( ( ',' >> lit("section") >> qi::attr(true) ) | attr(false) )
	  >> ')' 
	)
      ;
    
    r_modelstep  =  ( r_identifier >> ':' >> r_solidmodel_expression >> ';' ) 
      [ phx::bind(&Model::addModelstep, model_, qi::_1, qi::_2) ]
	;
    
    
    r_solidmodel_expression =
      r_solidmodel_term [_val=qi::_1 ]
      >>
       *( '-' >> r_solidmodel_term [ _val = construct<FeaturePtr>(new_<BooleanSubtract>(qi::_val, qi::_1)) ] )
      ;
    
    r_solidmodel_term =
      r_solidmodel_primary [_val=qi::_1 ]
      >>
       -( lit("<<") >> r_vectorExpression [ _val = construct<FeaturePtr>(new_<Transform>(qi::_val, qi::_1)) ] )
      >>
       -( lit("*") >> r_scalarExpression [ _val = construct<FeaturePtr>(new_<Transform>(qi::_val, qi::_1)) ] )
      >> *( 
      ('|' >> r_solidmodel_primary [ _val = construct<FeaturePtr>(new_<BooleanUnion>(_val, qi::_1)) ] )
      |
      ('&' >> r_solidmodel_primary [ _val = construct<FeaturePtr>(new_<BooleanIntersection>(_val, qi::_1)) ] )
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
	qi::lexeme [ model_->modelstepSymbols() ] 
	 [ _val =  phx::bind(&Model::lookupModelstep, model_, qi::_1) ]
      ;
#warning disabled for now!
//     r_submodel_modelstep =
//       qi::lexeme[ model_->modelSymbols() ] 
//        [ _a =  phx::bind(&Model::lookupModel, model_, qi::_1) ]
// 	>> lit('.') >>
// 	r_identifier [ _val =  phx::construct<FeaturePtr>(phx::new_<ModelFeature>(_a, qi::_1)) ]
// 	;
//       
    r_solidmodel_subshape =
       ( r_solidmodel_expression >> lit(':') >> r_identifier )
	 [ _val = phx::construct<FeaturePtr>(phx::new_<Subfeature>(qi::_1, qi::_2)) ]
	  ;

//     r_solidmodel_propertyAssignment =
//       qi::lexeme[ model_->modelstepSymbolNames() ] [ _a =  phx::bind(&Model::lookupModelstep, model_, qi::_1) ] 
// 	  >> lit("->") >
// 	   (
// 	     ( lit("CoG") > '=' > r_vectorExpression ) [ lazy( phx::bind(&SolidModel::setCoGExplicitly, *_a, qi::_1) ) ]
// 	     |
// 	     ( lit("mass") > '=' > r_scalarExpression ) [ lazy( phx::bind(&SolidModel::setMassExplicitly, *_a, qi::_1) ) ]
// 	     |
// 	     ( lit("density") > '=' > r_scalarExpression ) [ lazy( phx::bind(&SolidModel::setDensity, *_a, qi::_1) ) ]
// 	     |
// 	     ( lit("areaWeight") > '=' > r_scalarExpression ) [ lazy( phx::bind(&SolidModel::setAreaWeight, *_a, qi::_1) ) ]
// 	   )
// 	  > ';'
// 	  ;

    r_vertexFeaturesExpression = 
	  qi::lexeme[model_->vertexFeatureSymbols()] 
	    [ _val =  phx::bind(&Model::lookupVertexFeature, model_, qi::_1) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> (lit("vertices")|lit("vertex"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Vertex, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> lit("allvertices")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Vertex)) ]
      ;

    r_edgeFeaturesExpression = 
	  qi::lexeme[model_->edgeFeatureSymbols()] 
	    [ _val =  phx::bind(&Model::lookupEdgeFeature, model_, qi::_1) ]
	  | 
	  ( r_solidmodel_expression
	    >> '.'
	    >> (lit("egdes")|lit("edge"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_edgeFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Edge, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> lit("alledges")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Edge)) ]
      ;

    r_faceFeaturesExpression = 
	  qi::lexeme[model_->faceFeatureSymbols()] 
	    [ _val =  phx::bind(&Model::lookupFaceFeature, model_, qi::_1) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> (lit("faces")|lit("face"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Face, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> lit("allfaces")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Face)) ]
	;

    r_solidFeaturesExpression = 
	  qi::lexeme[model_->solidFeatureSymbols()] 
	    [ _val =  phx::bind(&Model::lookupSolidFeature, model_, qi::_1) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> (lit("solids")|lit("solid"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_solidFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Solid, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '.'
	    >> lit("allsolids")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Solid)) ]
	;

      
    r_datumExpression = 
	  qi::lexeme[model_->datumSymbols()] 
	    [ _val =  phx::bind(&Model::lookupDatum, model_, qi::_1) ]
	  |
	  ( lit("Plane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2)) ]
	  |
	  ( lit("SPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2, qi::_3)) ]
      ;
      
    r_scalarExpression = 
      r_scalar_term [ _val = qi::_1]  >> *(
	( '+' >> r_scalar_term [ _val = phx::construct<ScalarPtr>(phx::new_<AddedScalar>(qi::_val, qi::_1)) ] )
      | ( '-' >> r_scalar_term [ _val = phx::construct<ScalarPtr>(phx::new_<SubtractedScalar>(qi::_val, qi::_1)) ] )
      )
      ;
    
    r_scalar_term =
    (
      r_scalar_primary [ _val = qi::_1 ] >> 
      *(
	( '*' >> r_scalar_primary [ _val = phx::construct<ScalarPtr>(phx::new_<MultipliedScalar>(qi::_val, qi::_1)) ] )
	| 
	( '/' >> r_scalar_primary [ _val = phx::construct<ScalarPtr>(phx::new_<DividedScalar>(qi::_val, qi::_1)) ] )
       )
    ) 
    | 
    (
      r_vector_primary >> '&' >> r_vector_primary
    ) [ _val = phx::construct<ScalarPtr>(phx::new_<DotMultipliedVector>(qi::_1, qi::_2)) ]
      ;
      
    r_scalar_primary =
      qi::lexeme[model_->scalarSymbols()]
	[ _val = phx::bind(&Model::lookupScalar, model_, qi::_1) ]
      |
       ( r_solidmodel_expression >> '$' >> ( r_identifier | qi::attr(std::string()) ) ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<ScalarFeatureProp>(qi::_1, qi::_2)) ] 
        
      | double_ 
	[ _val = phx::construct<ScalarPtr>(phx::new_<ConstantScalar>(qi::_1)) ]
//       | ( lit("pow") >> '(' >> r_scalarExpression >> ',' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::pow, qi::_1, qi::_2) ]
//       | ( lit("sqrt") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::sqrt, qi::_1) ]
//       | ( lit("sin") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::sin, qi::_1) ]
//       | ( lit("cos") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::cos, qi::_1) ]
//       | ( lit("tan") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::tan, qi::_1) ]
//       | ( lit("asin") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::asin, qi::_1) ]
//       | ( lit("acos") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::acos, qi::_1) ]
//       | ( lit("atan") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::atan, qi::_1) ]
//       | ( lit("atan2") >> '(' >> r_scalarExpression >> ',' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::atan2, qi::_1, qi::_2) ]
      | ('(' >> r_scalarExpression >> ')') [ _val = qi::_1 ]
      
      | ( lit("TableLookup") > '(' > r_identifier > ',' 
	    > r_identifier > ',' > r_scalarExpression > ',' > r_identifier > ')' )
	[ _val = phx::construct<ScalarPtr>(phx::new_<LookupTableScalar>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
	
      | ( r_vector_primary >> '.' >> 'x' ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 0)) ]
      | ( r_vector_primary >> '.' >> 'y' ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 1)) ]
      | ( r_vector_primary >> '.' >> 'z' ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 2)) ]
      | ('-' >> r_scalar_primary) 
        [
         _val 
         =
         phx::construct<ScalarPtr>(phx::new_<MultipliedScalar>
         (
// 	  phx::construct<ScalarPtr>(phx::new_<ConstantScalar>(-1.0)),
	  ScalarPtr( new ConstantScalar(-1.0)),
	  qi::_1
	 ))
	]
      ;


    r_vectorExpression =
      r_vector_term [ _val = qi::_1 ]  >> 
      *(
	( '+' >> r_vector_term [_val=phx::construct<VectorPtr>(phx::new_<AddedVector>(qi::_val, qi::_1)) ] )
	| 
	( '-' >> r_vector_term [_val=phx::construct<VectorPtr>(phx::new_<SubtractedVector>(qi::_val, qi::_1)) ] )
      )
      ;
    
    r_vector_term =
    (
      r_vector_primary [_val=qi::_1] >> *(
	( '*' >> r_scalar_term [ _val=phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>(qi::_1, qi::_val)) ] )
      | ( '/' >> r_scalar_term [ _val=phx::construct<VectorPtr>(phx::new_<ScalarDividedVector>(qi::_val, qi::_1)) ] )
      | ( '^' >> r_vector_primary [ _val=phx::construct<VectorPtr>(phx::new_<CrossMultipliedVector>(qi::_val, qi::_1)) ] )
      )
    ) | (
      r_scalar_primary >> '*' >> r_vector_term
    )  [ _val = phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>(qi::_1, qi::_2)) ]
    ;
      
    r_vector_primary =
//        ( lit("modelCoG") )
//         [ _val = phx::bind(&Model::modelCoG, model_) ]
//       |
//        ( lit("rot") > '(' > r_vectorExpression > lit("by") > r_scalarExpression > ( (lit("around") > r_vectorExpression) | attr(vec3(0,0,1)) )> ')' )
//         [ _val = rot_(qi::_1, qi::_2, qi::_3) ]
//       |
       ( r_solidmodel_expression >> '@' >> ( r_identifier | qi::attr(std::string()) ) ) 
        [ _val = phx::construct<VectorPtr>(phx::new_<PointFeatureProp>(qi::_1, qi::_2)) ]
      |
       ( r_solidmodel_expression >> '!' >> ( r_identifier | qi::attr(std::string()) ) ) 
        [ _val = phx::construct<VectorPtr>(phx::new_<VectorFeatureProp>(qi::_1, qi::_2)) ]
      |
       qi::lexeme[model_->vectorSymbols()] 
        [ _val =  phx::bind(&Model::lookupVector, model_, qi::_1) ]
//       |
//       qi::lexeme[ model_->modelsteps() ] [ _a =  phx::bind(&Model::lookupModelstep, model_, qi::_1) ] 
// 	  >> lit("->") >
// 	   (
// 	     lit("CoG") [ lazy( _val = phx::bind(&getModelCoG, *_a)) ]
// 	   )
      |
       ( "[" >> r_scalarExpression >> "," >> r_scalarExpression >> "," >> r_scalarExpression >> "]" ) 
        [ _val = phx::construct<VectorPtr>(phx::new_<VectorFromComponents>(qi::_1, qi::_2, qi::_3)) ] 
      //| ( r_vectorExpression >> '\'') [ _val = trans_(qi::_1) ]
      |
       ( '(' >> r_vectorExpression >> ')' ) 
        [_val = qi::_1]
      |
       ( '-' >> r_vector_primary ) 
        [
         _val 
         =
         phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>
         (
	  ScalarPtr( new ConstantScalar(-1.0)),
// 	  phx::construct<VectorPtr>(phx::new_<ConstantScalar>(-1.0)),
	  qi::_1
	 ))
	]
      ;

    for (Feature::FactoryTable::const_iterator i = Feature::factories_->begin();
	i != Feature::factories_->end(); i++)
    {
      FeaturePtr sm(i->second->operator()(NoParameters()));
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



bool parseISCADModel(std::string::iterator first, std::string::iterator last, Model* model)
{
  ISCADParser parser(model);
  skip_grammar skip;
  
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  
  if (first != last) return false;
  return r;
}

}

using namespace parser;


bool parseISCADModelFile(const boost::filesystem::path& fn, Model* m)
{
  std::ifstream f(fn.c_str());
  return parseISCADModelStream(f, m);
}

bool parseISCADModelStream(std::istream& in, Model* m, int* failloc)
{
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  
  std::string::iterator orgbegin,
    first=contents_raw.begin(), 
    last=contents_raw.end();
    
  orgbegin=first;
  
  ISCADParser parser(m);
  skip_grammar skip;
  
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  
  if (first != last) // fail if we did not get a full match
  {
    if (failloc) *failloc=int(first-orgbegin);
    return false;
  }
  
  return r;
}

}
}
