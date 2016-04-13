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

#include "cadtypes.h"
#ifdef INSIGHT_CAD_DEBUG
#define BOOST_SPIRIT_DEBUG
#endif

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

namespace parser {  


using namespace qi;
using namespace phx;
using namespace insight::cad;




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



/*! \page cad_parser ISCAD Parser Language
 * 
 * ISCAD language knows the following commands:
 * - \subpage Arc
 * - \subpage Bar
 * - \subpage BooleanIntersection
 * - \subpage BooleanSubtract
 * - \subpage BooleanUnion
 * - \subpage BoundedFlatFace
 * - \subpage Box
 * - \subpage Chamfer
 * - \subpage Circle
 * - \subpage CircularPattern
 * - \subpage Coil
 * - \subpage Compound
 * - \subpage Cone
 * - \subpage Cutaway
 * - \subpage Cylinder
 * - \subpage Extrusion
 * - \subpage Fillet
 * - \subpage FillingFace
 * - \subpage ImportSolid
 * - \subpage Line
 * - \subpage LinearPattern
 * - \subpage Mirror
 * - \subpage ModelFeature
 * - \subpage NacaFourDigit
 * - \subpage Pipe
 * - \subpage Place
 * - \subpage Projected
 * - \subpage ProjectedOutline
 * - \subpage Quad
 * - \subpage RegPoly
 * - \subpage Revolution
 * - \subpage Ring
 * - \subpage RotatedHelicalSweep
 * - \subpage Shoulder
 * - \subpage Sphere
 * - \subpage SplineCurve
 * - \subpage SplineSurface
 * - \subpage Split
 * - \subpage Spring
 * - \subpage StitchedShell
 * - \subpage StitchedSolid
 * - \subpage Subfeature
 * - \subpage Sweep 
 * - \subpage Thicken
 * - \subpage Torus
 * - \subpage Transform
 * - \subpage Tri
 * - \subpage Wire
 */

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
       > *r_postproc
      )
     ;
    
     
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
      ( r_identifier >> '=' >> r_solidmodel_expression >> ';' ) 
        [ phx::bind(&Model::addModelstep, model_, qi::_1, qi::_2) ]
      |
      ( r_identifier >> '='  >> r_datumExpression >> ';') 
	[ phx::bind(&Model::addDatum, model_, qi::_1, qi::_2) ]
      |
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
      ;
      
/** @addtogroup cad_parser
  * @{
  * @section postproc Postprocessing statements
  * 
  * @subsection DXF Save DXF drawing.
  * 
  * Syntax:
  * ~~~~
  * DXF("<filename>") << <feature expression>
  *     <viewname> (<vector viewon>, <vector viewfrom> [, up <vector upward direction>] [, section] [, poly]) 
  *     [<viewname> ...]
  * ;
  * ~~~~
  * 
  * @subsection saveAs Export feature into file.
  * 
  * Syntax:Create a fille
  * ~~~~
  * saveAs("<filename>") << <feature expression>;
  * ~~~~
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
	  >> ( ( ',' >> lit("up") >> r_vectorExpression ) | attr(VectorPtr()) )
	  >> ( ( ',' >> lit("section") >> qi::attr(true) ) | attr(false) )
	  >> ( ( ',' >> lit("poly") >> qi::attr(true) ) | attr(false) )
	  >> ')' 
	)
      ;
    
    r_modelstep  =  ( r_identifier >> ':' >> r_solidmodel_expression > ';' ) 
      [ phx::bind(&Model::addComponent, model_, qi::_1, qi::_2) ]
	;
    
    
    r_solidmodel_expression =
      r_solidmodel_term [_val=qi::_1 ]
      >>
       *( '-' >> r_solidmodel_term [ _val = construct<FeaturePtr>(new_<BooleanSubtract>(qi::_val, qi::_1)) ] )
      ;
    
    r_solidmodel_term =
      r_solidmodel_primary [_val=qi::_1 ]
      >> 
       -( '.' > r_identifier
	 [ _val = phx::construct<FeaturePtr>(phx::new_<Subfeature>(qi::_val, qi::_1)) ] )
      >>
       -( lit("<<") > r_vectorExpression [ _val = construct<FeaturePtr>(new_<Transform>(qi::_val, qi::_1)) ] )
      >>
       -( lit("*") > r_scalarExpression [ _val = construct<FeaturePtr>(new_<Transform>(qi::_val, qi::_1)) ] )
      >>
      *( 
        ('|' > r_solidmodel_primary [ _val = construct<FeaturePtr>(new_<BooleanUnion>(_val, qi::_1)) ] )
	|
	('&' > r_solidmodel_primary [ _val = construct<FeaturePtr>(new_<BooleanIntersection>(_val, qi::_1)) ] )
       )
      ;

    r_modelstepFunction %= omit [ modelstepFunctionRules[ qi::_a = qi::_1 ] ] > qi::lazy(*qi::_a);
    
    r_solidmodel_primary = 
       r_modelstepFunction 
        [ _val = qi::_1 ]
      |
//        qi::lexeme [ model_->modelstepSymbols() ] 
// 	[ _val =  phx::bind(&Model::lookupModelstep, model_, qi::_1) ]
        model_->modelstepSymbols()[_val=qi::_1 ]
      |
       ( '(' >> r_solidmodel_expression 
        [ _val = qi::_1] > ')' )
      // try identifiers last, since exceptions are generated, if symbols don't exist
      ;

    r_solidmodel_propertyAssignment =
      qi::lexeme[ model_->modelstepSymbols() ] [ _a = qi::_1 ] 
	  >> lit("->") >
	   (
// 	     ( lit("CoG") > '=' > r_vectorExpression ) [ lazy( phx::bind(&Feature::setCoGExplicitly, *_a, qi::_1) ) ]
// 	     |
// 	     ( lit("mass") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setMassExplicitly, *_a, qi::_1) ) ]
// 	     |
	     ( lit("density") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setDensity, *_a, qi::_1) ) ]
	     |
	     ( lit("areaWeight") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setAreaWeight, *_a, qi::_1) ) ]
	     |
	     ( lit("visresolution") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setVisResolution, *_a, qi::_1) ) ]
	   )
	  > ';'
	  ;

    r_vertexFeaturesExpression = 
	 (
	   model_->vertexFeatureSymbols() [ qi::_val = qi::_1 ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> (lit("vertices")|lit("vertex"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Vertex, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> lit("allvertices")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Vertex)) ]
	 )
	 >>
	  *(
	       '?'
	    >> (lit("vertices")|lit("vertex"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
      ;

    r_edgeFeaturesExpression =
         (
	  model_->edgeFeatureSymbols() [ _val = qi::_1 ]
	  | 
	  ( r_solidmodel_expression
	    >> '?'
	    >> (lit("edges")|lit("edge"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_edgeFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Edge, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> lit("alledges")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Edge)) ]
	 )
	 >>
	  *(
	       '?'
	    >> (lit("edges")|lit("edge"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_edgeFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
      ;

    r_faceFeaturesExpression = 
         (
	  model_->faceFeatureSymbols()[ qi::_val = qi::_1 ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> (lit("faces")|lit("face"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Face, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> lit("allfaces")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Face)) ]
	 )
	 >>
	  *(
	       '?'
	    >> (lit("faces")|lit("face"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
	;

    r_solidFeaturesExpression = 
         (
	  model_->solidFeatureSymbols()[ qi::_val = qi::_1 ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> (lit("solids")|lit("solid"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_solidFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Solid, qi::_2, qi::_3)) ]
	  |
	  ( r_solidmodel_expression
	    >> '?'
	    >> lit("allsolids")
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Solid)) ]
	 )
	 >>
	  *(
	       '?'
	    >> (lit("solids")|lit("solid"))
	    >> '(' 
	    >> r_string 
	    >> *( ',' >> (r_solidFeaturesExpression|r_vectorExpression|r_scalarExpression) )
	    >> ')' 
	  ) 
	   [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
	;

      
    r_datumExpression = 
// 	  qi::lexeme[model_->datumSymbols()] 
// 	    [ _val =  phx::bind(&Model::lookupDatum, model_, qi::_1) ]
          model_->datumSymbols()[ qi::_val = qi::_1 ]
	  |
	  ( lit("Plane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2)) ]
	  |
	  ( lit("SPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2, qi::_3)) ]
	  |
	  ( lit("TPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2, qi::_3, true)) ]
	  |
	  ( lit("RefPt") >> '(' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<ExplicitDatumPoint>(qi::_1)) ]
	  |
	  ( lit("RefAxis") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<ExplicitDatumAxis>(qi::_1, qi::_2)) ]
	  |
	  ( lit("xsec_axpl") >> '(' >> r_datumExpression >> ',' >> r_datumExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<XsecAxisPlane>(qi::_1, qi::_2)) ]
	  |
	  ( lit("xsec_plpl") >> '(' >> r_datumExpression >> ',' >> r_datumExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<XsecPlanePlane>(qi::_1, qi::_2)) ]
	  |
	  ( lit("xsec_ppp") >> '(' >> r_datumExpression >> ',' >> r_datumExpression >> ',' >> r_datumExpression >> ')' ) 
	    [ _val = construct<DatumPtr>(new_<XsecAxisPlane>(
	               construct<DatumPtr>(new_<XsecPlanePlane>(qi::_1, qi::_2)),
		       qi::_3)) ]
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
      double_ 
	[ _val = phx::construct<ScalarPtr>(phx::new_<ConstantScalar>(qi::_1)) ]
      |
//        qi::lexeme[model_->scalarSymbols()]
// 	[ _val = phx::bind(&Model::lookupScalar, model_, qi::_1) ]        
        model_->scalarSymbols()[ qi::_val = qi::_1 ]
      | ( lit("volume") > '(' > r_solidmodel_expression > ')' ) 
	[ _val = phx::construct<ScalarPtr>(phx::new_<FeatureVolume>(qi::_1)) ]
      | ( lit("sqrt") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sqrt>(qi::_1)) ]
      | ( lit("sin") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sin>(qi::_1)) ]
      | ( lit("cos") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_cos>(qi::_1)) ]
      | ( lit("tan") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_tan>(qi::_1)) ]
      | ( lit("asin") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_asin>(qi::_1)) ]
      | ( lit("acos") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_acos>(qi::_1)) ]
      | ( lit("atan") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_atan>(qi::_1)) ]
      | ( lit("ceil") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_ceil>(qi::_1)) ]
      | ( lit("floor") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_floor>(qi::_1)) ]
      | ( lit("round") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_round>(qi::_1)) ]
      | ( lit("pow") > '(' > r_scalarExpression > ',' > r_scalarExpression > ')' ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_pow>(qi::_1, qi::_2)) ]
      | ( lit("atan2") > '(' > r_scalarExpression > ',' > r_scalarExpression > ')' ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_atan2>(qi::_1, qi::_2)) ]
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
	  ScalarPtr( new ConstantScalar(-1.0)),
	  qi::_1
	 ))
	]
      |
       ( r_solidmodel_expression >> '$' >> ( r_identifier | qi::attr(std::string()) ) ) 
        [ _val = phx::construct<ScalarPtr>(phx::new_<ScalarFeatureProp>(qi::_1, qi::_2)) ] 
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
	( '*' >> r_scalar_term 
	 [ _val=phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>(qi::_1, qi::_val)) ] )
      | ( '/' >> r_scalar_term 
         [ _val=phx::construct<VectorPtr>(phx::new_<ScalarDividedVector>(qi::_val, qi::_1)) ] )
      | ( '^' >> r_vector_primary 
         [ _val=phx::construct<VectorPtr>(phx::new_<CrossMultipliedVector>(qi::_val, qi::_1)) ] )
      | lit(">>") > 
       (
	( '(' >> r_datumExpression >> ',' >> r_vectorExpression >> ')' )
	 [ _val = phx::construct<VectorPtr>(phx::new_<ProjectedPoint>(qi::_val, qi::_1, qi::_2)) ] 
	|
        r_datumExpression
	 [ _val = phx::construct<VectorPtr>(phx::new_<ProjectedPoint>(qi::_val, qi::_1)) ] 
       )
      )
    ) | (
      r_scalar_primary >> '*' >> r_vector_term
    )  [ _val = phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>(qi::_1, qi::_2)) ]
    ;
      
    r_vector_primary =
//        ( lit("modelCoG") )
//         [ _val = phx::bind(&Model::modelCoG, model_) ]
//       |
       ( lit("rot") > '(' 
          > r_vectorExpression 
          > lit("by") > r_scalarExpression 
          > ( (lit("around") > r_vectorExpression) | attr(VectorPtr( new ConstantVector(vec3(0,0,1)))) )> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<RotatedVector>(qi::_1, qi::_2, qi::_3)) ]
      |
//        qi::lexeme[model_->vectorSymbols()] 
//         [ _val =  phx::bind(&Model::lookupVector, model_, qi::_1) ]
       model_->vectorSymbols()[ qi::_val = qi::_1 ]
//       qi::lexeme[ model_->modelsteps() ] [ _a =  phx::bind(&Model::lookupModelstep, model_, qi::_1) ] 
// 	  >> lit("->") >
// 	   (
// 	     lit("CoG") [ lazy( _val = phx::bind(&getModelCoG, *_a)) ]
// 	   )
      |
       ( lit("coord") >> '(' >> r_vertexFeaturesExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<SinglePointCoords>(qi::_1)) ]
      |
       ( lit("refpt") >> '(' >> r_datumExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumPointCoord>(qi::_1)) ]
      |
       ( lit("refdir") >> '(' >> r_datumExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumDir>(qi::_1)) ]
      |
       ( lit("plnorm") >> '(' >> r_datumExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumPlaneNormal>(qi::_1)) ]
      |
       ( lit("circcenter") >> '(' >> r_edgeFeaturesExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<CircleEdgeCenterCoords>(qi::_1)) ]
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
	  qi::_1
	 ))
	]
	|
	( r_solidmodel_expression >> '@' >> ( r_identifier | qi::attr(std::string()) ) ) 
	  [ _val = phx::construct<VectorPtr>(phx::new_<PointFeatureProp>(qi::_1, qi::_2)) ]
	|
	( r_solidmodel_expression >> '!' >> ( r_identifier | qi::attr(std::string()) ) ) 
	  [ _val = phx::construct<VectorPtr>(phx::new_<VectorFeatureProp>(qi::_1, qi::_2)) ]
      ;

    for (Feature::FactoryTable::const_iterator i = Feature::factories_->begin();
	i != Feature::factories_->end(); i++)
    {
      FeaturePtr sm(i->second->operator()(NoParameters()));
      sm->insertrule(*this);
    }
    
#if (INSIGHT_CAD_DEBUG>2)
	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_expression);
	BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);
	BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
	BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
	BOOST_SPIRIT_DEBUG_NODE(r_modelstepFunction);
	BOOST_SPIRIT_DEBUG_NODE(r_path);
	BOOST_SPIRIT_DEBUG_NODE(r_identifier);
	BOOST_SPIRIT_DEBUG_NODE(r_assignment);
	BOOST_SPIRIT_DEBUG_NODE(r_postproc);
	BOOST_SPIRIT_DEBUG_NODE(r_viewDef);
	BOOST_SPIRIT_DEBUG_NODE(r_scalarExpression);
	BOOST_SPIRIT_DEBUG_NODE(r_vector_primary);
	BOOST_SPIRIT_DEBUG_NODE(r_vector_term);
	BOOST_SPIRIT_DEBUG_NODE(r_vectorExpression);
	BOOST_SPIRIT_DEBUG_NODE(r_model);
	BOOST_SPIRIT_DEBUG_NODE(r_vertexFeaturesExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_propertyAssignment);
	BOOST_SPIRIT_DEBUG_NODE(r_edgeFeaturesExpression);
#endif
	
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
  
  std::cout<<"Parsing started."<<std::endl;
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  std::cout<<"Parsing finished."<<std::endl;
  
  if (first != last) return false;
  return r;
}

}

using namespace parser;


bool parseISCADModelFile(const boost::filesystem::path& fn, Model* m, int* failloc)
{
  std::ifstream f(fn.c_str());
  return parseISCADModelStream(f, m, failloc);
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
  
  std::cout<<"Parsing started."<<std::endl;
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  std::cout<<"Parsing finished."<<std::endl;
  
  if (first != last) // fail if we did not get a full match
  {
    if (failloc) *failloc=int(first-orgbegin);
    return false;
  }
  
  return r;
}

}
}
