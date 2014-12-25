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

#include "parser.h"
#include "boost/locale.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/foreach.hpp"
#include "boost/filesystem.hpp"

#include "dxfwriter.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;


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
  scalarSymbols.add( "M_PI", M_PI );
  vectorSymbols.add( "O", vec3(0,0,0) );
  vectorSymbols.add( "EX", vec3(1,0,0) );
  vectorSymbols.add( "EY", vec3(0,1,0) );
  vectorSymbols.add( "EZ", vec3(0,0,1) );
  datumSymbols.add( "XY", Datum::Ptr(new DatumPlane(vec3(0,0,0), vec3(0,0,1), vec3(0,1,0))) );
  datumSymbols.add( "XZ", Datum::Ptr(new DatumPlane(vec3(0,0,0), vec3(0,1,0), vec3(1,0,0))) );
  datumSymbols.add( "YZ", Datum::Ptr(new DatumPlane(vec3(0,0,0), vec3(1,0,0), vec3(0,1,0))) );
  
  BOOST_FOREACH(const ModelSymbols::value_type& s, syms)
  {
    const std::string& name=boost::fusion::at_c<0>(s);
    cout<<"Insert symbol:"<<name<<endl;
    if ( const scalar* sv = boost::get<scalar>( &boost::fusion::at_c<1>(s) ) )
    {
        scalarSymbols.add(name, *sv);
	cout<<(*sv)<<endl;
    }
    else if ( const vector* vv = boost::get<vector>( &boost::fusion::at_c<1>(s) ) )
    {
        vectorSymbols.add(name, *vv);
	cout<<(*vv)<<endl;
    }
  }
}

// solidmodel import(const boost::filesystem::path& filepath)
// {
//   cout << "reading model "<<filepath<<endl;
//   return solidmodel(new SolidModel(filepath));
// }

double dot(const vector& v1, const vector& v2)
{
  return arma::as_scalar(arma::dot(v1,v2));
}

FeatureSet queryEdges(const SolidModel& m, const FilterPtr& f)
{
  using namespace std;
  using namespace insight::cad;
  return m.query_edges(f);
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
  std::vector<std::string> paths;
  const char* e=getenv("ISCAD_MODEL_PATH");
  if (e)
  {
    boost::split(paths, e, boost::is_any_of(":"));
  }
  paths.insert(paths.begin(), ".");
  
  BOOST_FOREACH(const std::string& ps, paths)
  {
    path p(ps); 
    p=p/name;
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
  cout<<"lookup "<<key<<" in "<<map.size()<<endl;
  typename Map::const_iterator i=map.find(key);
  if (i!=map.end())
    return T(i->second);
  else
    return T();
}

template <typename Iterator>
struct skip_grammar : public qi::grammar<Iterator>
{
        skip_grammar() : skip_grammar::base_type(skip, "PL/0")
        {
            skip
                =   boost::spirit::ascii::space
                | repo::confix("/*", "*/")[*(qi::char_ - "*/")]
                | repo::confix("//", qi::eol)[*(qi::char_ - qi::eol)]
                | repo::confix("#", qi::eol)[*(qi::char_ - qi::eol)]
                ;
        }

        qi::rule<Iterator> skip;

};


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

template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct ISCADParser
  : qi::grammar<Iterator, Skipper>
{

  Model::Ptr model_;

    ISCADParser(Model::Ptr model)
      : ISCADParser::base_type(r_model),
	model_(model)
    {
      
        r_model =  *( r_assignment | r_modelstep | r_loadmodel ) 
		  >> -( lit("@post")  >> *r_postproc);
	
	r_loadmodel = ( lit("import") >> '(' >> r_identifier >> 
	  *(',' >> (r_identifier >> '=' >> (r_scalarExpression|r_vectorExpression) ) ) >> ')' 
// 	   >> -( lit("as") > r_identifier )
	   >> ';' )
	  [ phx::bind(model_->modelSymbols.add, _1, loadModel_(_1, _2)) ];
	
	r_assignment = 
	  ( r_identifier >> '='  >> r_scalarExpression >> ';') 
	   [ phx::bind(model_->scalarSymbols.add, _1, _2) ]
	  |
	  ( r_identifier >> lit("?=")  >> r_scalarExpression >> ';') 
	   [ phx::bind(addSymbolIfNotPresent<double>, phx::ref(model_->scalarSymbols), _1, _2) ]
	  |
	  ( r_identifier >> '='  >> r_vectorExpression >> ';') 
	   [ phx::bind(model_->vectorSymbols.add, _1, _2) ]
	  |
	  ( r_identifier >> lit("?=")  >> r_vectorExpression >> ';') 
	   [ phx::bind(addSymbolIfNotPresent<vector>, phx::ref(model_->vectorSymbols), _1, _2) ]
	  |
// 	  ( r_identifier >> '='  >> r_edgeFeaturesExpression >> ';') [ phx::bind(model_->edgeFeatureSymbols.add, _1, _2) ]
// 	  |
	  ( r_identifier >> '='  >> r_datumExpression >> ';') 
	   [ phx::bind(model_->datumSymbols.add, _1, _2) ]
	  ;
	  
	r_postproc =
	  ( lit("DXF") >> '(' >> r_path >> ')' >> lit("<<") >> r_solidmodel_expression >> *(r_viewDef) >> ';' ) [ writeViews_(_1, _2, _3) ]
	  ;
	  
	r_viewDef =
	   (r_identifier >> '(' 
	      >> r_vectorExpression >> ',' 
	      >> r_vectorExpression
	      >> -( ',' >> lit("section") >> qi::attr(true) )
	      >> ')' 
	   )
	  ;
	
        r_modelstep  =  ( r_identifier >> ':' > r_solidmodel_expression > ';' ) 
	  [ phx::bind(model_->modelstepSymbols.add, _1, _2) ];
	// this does not work: (nothing inserted)
// 	  [ phx::insert
// 	    (
// 	      model_->modelstepSymbols, 
// 	      phx::construct<std::pair<std::string, SolidModel::Ptr> >(_1, _2)
// 	    ) ]
	// this works:
// 	  [ phx::ref(model_->modelstepSymbols)[_1] = _2 ]
	    ;
	
	
	r_solidmodel_expression =
	 r_solidmodel_term [_val=_1 ]
	 >> *( '-' >> r_solidmodel_term [_val=construct<solidmodel>(new_<BooleanSubtract>(*_val, *_1))] )
	 ;
	
	r_solidmodel_term =
	 r_solidmodel_primary [_val=_1 ]
	 >> *( '|' >> r_solidmodel_primary [_val=construct<solidmodel>(new_<BooleanUnion>(*_val, *_1))] )
	 ;
	
	r_solidmodel_primary = 
// 	 lexeme[ model_->modelstepSymbols >> !(alnum | '_') ] [ _val = _1 ]
	 ( '(' >> r_solidmodel_expression [_val=_1] > ')' )
	 
         | ( lit("import") > '(' > r_path > ')' ) [ _val = construct<solidmodel>(new_<SolidModel>(_1)) ]
         
         | ( lit("Tri") > '(' > r_vectorExpression > ',' > r_vectorExpression > ',' > r_vectorExpression > ')' ) 
	    [ _val = construct<solidmodel>(new_<Tri>(_1, _2, _3)) ]
         | ( lit("Quad") > '(' > r_vectorExpression > ',' > r_vectorExpression > ',' > r_vectorExpression > ')' ) 
	    [ _val = construct<solidmodel>(new_<Quad>(_1, _2, _3)) ]
         | ( lit("Circle") > '(' > r_vectorExpression > ',' > r_vectorExpression > ',' > r_scalarExpression > ')' ) 
	    [ _val = construct<solidmodel>(new_<Circle>(_1, _2, _3)) ]
         | ( lit("RegPoly") > '(' > r_vectorExpression > ',' > r_vectorExpression 
				  > ',' > r_scalarExpression > ',' > r_scalarExpression 
				  > ( (',' > r_vectorExpression)|attr(arma::mat()) ) > ')' ) 
	    [ _val = construct<solidmodel>(new_<RegPoly>(_1, _2, _3, _4, _5)) ]
         | ( lit("Sketch") > '(' > r_datumExpression > ',' > r_path > ',' > r_string > ')' ) 
	    [ _val = construct<solidmodel>(new_<Sketch>(*_1, _2, _3)) ]
         
         | ( lit("CircularPattern") > '(' > r_solidmodel_expression > ',' > r_vectorExpression > ',' 
	    > r_vectorExpression > ',' > r_scalarExpression > ')' ) 
	     [ _val = construct<solidmodel>(new_<CircularPattern>(*_1, _2, _3, _4)) ]
         | ( lit("LinearPattern") > '(' > r_solidmodel_expression > ',' > r_vectorExpression > ',' 
	    > r_scalarExpression > ')' ) 
	     [ _val = construct<solidmodel>(new_<LinearPattern>(*_1, _2, _3)) ]
         
         | ( lit("Transform") > '(' > r_solidmodel_expression > ',' > r_vectorExpression > ',' 
	    > r_vectorExpression > ')' ) 
	     [ _val = construct<solidmodel>(new_<Transform>(*_1, _2, _3)) ]
         | ( lit("Compound") > '(' > ( r_solidmodel_expression % ',' ) > ')' ) 
	     [ _val = construct<solidmodel>(new_<Compound>(_1)) ]

	 // Primitives
	 | ( lit("Sphere") > '(' > r_vectorExpression > ',' > r_scalarExpression > ')' ) 
	      [ _val = construct<solidmodel>(new_<Sphere>(_1, _2)) ]
	 | ( lit("Cylinder") > '(' > r_vectorExpression > ',' > r_vectorExpression > ',' > r_scalarExpression > ')' ) 
	      [ _val = construct<solidmodel>(new_<Cylinder>(_1, _2, _3)) ]
	 | ( lit("Box") > '(' > r_vectorExpression > ',' > r_vectorExpression 
			> ',' > r_vectorExpression > ',' > r_vectorExpression > -(  ',' > lit("centered") > attr(true) ) > ')' ) 
	      [ _val = construct<solidmodel>(new_<Box>(_1, _2, _3, _4, _5)) ]
// 	 | ( lit("Fillet") > '(' >> r_solidmodel_expression >> ',' >> r_edgeFeaturesExpression >> ',' >> r_scalarExpression >> ')' ) 
// 	      [ _val = construct<solidmodel>(new_<Fillet>(*_1, _2, _3)) ]
// 	 | ( lit("Chamfer") > '(' >> r_solidmodel_expression >> ',' >> r_edgeFeaturesExpression >> ',' >> r_scalarExpression >> ')' ) 
// 	      [ _val = construct<solidmodel>(new_<Chamfer>(*_1, _2, _3)) ]
	 | ( lit("Extrusion") > '(' > r_solidmodel_expression > ',' > r_vectorExpression > -(  ',' > lit("centered") > attr(true) ) > ')' ) 
	      [ _val = construct<solidmodel>(new_<Extrusion>(*_1, _2, _3)) ]
	 | ( lit("Revolution") > '(' > r_solidmodel_expression > ',' > r_vectorExpression > ',' > r_vectorExpression > ',' > r_scalarExpression > -(  ',' > lit("centered") > attr(true) ) > ')' ) 
	      [ _val = construct<solidmodel>(new_<Revolution>(*_1, _2, _3, _4, _5)) ]
	 | ( lit("RotatedHelicalSweep") > '(' 
		> r_solidmodel_expression > ',' 
		> r_vectorExpression > ',' 
		> r_vectorExpression > ',' 
		> r_scalarExpression > 
		((  ',' > r_scalarExpression ) | attr(0.0)) > ')' ) 
	      [ _val = construct<solidmodel>(new_<RotatedHelicalSweep>(*_1, _2, _3, _4, _5)) ]
	 | ( lit("Projected") > '(' > r_solidmodel_expression > ',' > r_solidmodel_expression > ',' > r_vectorExpression > ')' ) 
	      [ _val = construct<solidmodel>(new_<Projected>(*_1, *_2, _3)) ]
	 | ( lit("Split") > '(' > r_solidmodel_expression > ',' > r_solidmodel_expression > ')' ) 
	      [ _val = construct<solidmodel>(new_<Split>(*_1, *_2)) ]
	      
	 // try identifiers last, since exceptions are generated, if symbols don't exist
	 | 
	    r_solidmodel_subshape [ _val = _1 ]
	 | 
	    r_submodel_modelstep [ _val = _1 ]

	 |
	   model_->modelstepSymbols [ _val = _1 ]
// 	      [ _val = phx::at(phx::ref(model_->modelstepSymbols), _1) ]
// 	      [ _val = phx::bind(&lookupMap<SolidModel::Ptr>, model_->modelstepSymbols, _1) ]

// 	 | ( r_identifier >> '!' > r_identifier ) 
// 	      [ _val = phx::bind(&Model::lookupModelModelstep, *model_, _1, _2) ]
	 ;
	 
	r_submodel_modelstep =
	  ( model_->modelSymbols >> '.' ) [ _a = _1 ]
	   > lazy( phx::val(phx::bind(&Model::modelstepSymbols, *_a)) )
	    [ _val = _1 ]
	   ;
	 
	r_solidmodel_subshape =
	  ( model_->modelstepSymbols >> '.' ) [ _a = _1 ] 
	      > 
	      lazy( phx::val(phx::bind(&SolidModel::providedSubshapes, *_a)) )
		[ _val = _1 ]
	      ;

	 
// 	r_edgeFeaturesExpression = 
// 	     lexeme[ model_->edgeFeatureSymbols >> !(alnum | '_') ] [ _val = _1 ]
// 	     | (
// 	     ( lit("edgesFrom") >> 
// 		r_solidmodel_expression >> lit("where") >> r_edgeFilterExpression ) [ _val = queryEdges_(*_1, _2) ]
// 	     )
// 	  ;
// 	  
// 	r_edgeFilterExpression = 
// 	  ( lit("*") [ _val = construct<Filter::Ptr>(new_<everything>()) ] )
// 	 | ( lit("all") ) [ _val = construct<Filter::Ptr>(new_<everything>()) ]
// 	 | ( lit("coincidentWith") >> r_edgeFeaturesExpression >> lit("from") >> r_solidmodel_expression ) 
// 	    [ _val = construct<Filter::Ptr>(new_<coincident<Edge> >(*_2, _1)) ]
// 	 | ( lit("secant") >> r_vectorExpression ) [ _val = construct<Filter::Ptr>(new_<secant<Edge> >(_1)) ]
// 	 ;
	 
	r_datumExpression = 
	     lexeme[ model_->datumSymbols >> !(alnum | '_') ] [ _val = _1 ]
	     |
	     ( lit("Plane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
		[ _val = construct<Datum::Ptr>(new_<DatumPlane>(_1, _2)) ]
	     |
	     ( lit("SPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' ) 
		[ _val = construct<Datum::Ptr>(new_<DatumPlane>(_1, _2, _3)) ]
	  ;
	  
	r_path = as_string[ 
                            lexeme [ "\"" >> *~char_("\"") >> "\"" ] 
                         ];
	r_string = as_string[ 
                            lexeme [ "\'" >> *~char_("\'") >> "\'" ] 
                         ];
			 
	r_scalarExpression = 
	  r_scalar_term [_val =_1]  >> *(
	    ( '+' >> r_scalar_term [_val+=_1] )
	  | ( '-' >> r_scalar_term [_val-=_1] )
	  )
	  ;
	
	r_scalar_term =
	(
	  r_scalar_primary [_val=_1] >> *(
	    ( '*' >> r_scalar_primary [ _val*=_1 ] )
	  | ( '/' >> r_scalar_primary [ _val/=_1 ] )
	  )
	) | (
	  r_vector_primary >> '&' >> r_vector_primary
	) [_val = dot_(_1, _2) ]
	  ;
	  
	r_scalar_primary =
	  lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  | double_ [ _val = _1 ]
	  | ( lit("sin") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::sin, _1) ]
	  | ( lit("cos") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::cos, _1) ]
	  | ( lit("tan") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::tan, _1) ]
	  | ( lit("asin") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::asin, _1) ]
	  | ( lit("acos") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::acos, _1) ]
	  | ( lit("atan") >> '(' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::atan, _1) ]
	  | ( lit("atan2") >> '(' >> r_scalarExpression >> ',' >> r_scalarExpression >> ')' ) [ _val = phx::bind(&::atan2, _1, _2) ]
	  | ('(' >> r_scalarExpression >> ')') [_val=_1]
	  | ( lit("TableLookup") > '(' > r_identifier > ',' 
		> r_identifier > ',' > r_scalarExpression > ',' > r_identifier > ')' )
	    [ _val = lookupTable_(_1, _2, _3, _4) ]
	  | ('-' >> r_scalar_primary) [_val=-_1]
	  ;


	r_vectorExpression =
	  r_vector_term [_val =_1]  >> *(
	    ( '+' >> r_vector_term [_val+=_1] )
	  | ( '-' >> r_vector_term [_val-=_1] )
	  )
	  ;
	
	r_vector_term =
	(
	  r_vector_primary [_val=_1] >> *(
	    ( '*' >> r_scalar_term [ _val*=_1 ] )
	  | ( '/' >> r_scalar_term [ _val/=_1 ] )
	  | ( '^' >> r_vector_primary [ _val=cross_(_val, _1) ] )
	  )
	) | (
	  r_scalar_primary >> '*' >> r_vector_term
	) [_val=_1*_2]
	;
	  
	r_vector_primary =
	  lexeme[ model_->vectorSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  | ( "[" >> r_scalarExpression >> "," >> r_scalarExpression >> "," >> r_scalarExpression >> "]" ) [ _val = vec3_(_1, _2, _3) ] 
	  //| ( r_vectorExpression >> '\'') [ _val = trans_(_1) ]
	  | ( '(' >> r_vectorExpression >> ')' ) [_val=_1]
	  | ( '-' >> r_vector_primary ) [_val=-_1]
	  ;

	r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
	 
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
// 	BOOST_SPIRIT_DEBUG_NODE(r_edgeFilterExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_expression);
//	BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
// 	BOOST_SPIRIT_DEBUG_NODE(r_model);
	
        on_error<fail>(r_model, 
                phx::ref(std::cout)
                   << "Error! Expecting "
                   << qi::_4
                   << " here: '"
                   << phx::construct<std::string>(qi::_3, qi::_2)
                   << "'\n"
            );
    }
    
//     qi::rule<Iterator, int(), Skipper> r_int_primary, r_int_term, r_intExpression;
    qi::rule<Iterator, scalar(), Skipper> r_scalar_primary, r_scalar_term, r_scalarExpression;
    qi::rule<Iterator, vector(), Skipper> r_vector_primary, r_vector_term, r_vectorExpression;
    
//     qi::rule<Iterator, FeatureSet(), Skipper> r_edgeFeaturesExpression;
//     qi::rule<Iterator, Filter::Ptr(), Skipper> r_edgeFilterExpression;
    qi::rule<Iterator, datum(), Skipper> r_datumExpression;
    
    qi::rule<Iterator, Skipper> r_model;
    qi::rule<Iterator, Skipper> r_loadmodel;
    qi::rule<Iterator, Skipper> r_assignment;
    qi::rule<Iterator, Skipper> r_postproc;
    qi::rule<Iterator, viewdef(), Skipper> r_viewDef;
    qi::rule<Iterator, modelstep(), Skipper> r_modelstep;
    qi::rule<Iterator, std::string()> r_identifier;
    qi::rule<Iterator, std::string()> r_string;
    qi::rule<Iterator, boost::filesystem::path()> r_path;
    qi::rule<Iterator, solidmodel(), Skipper> r_solidmodel_primary, r_solidmodel_term, r_solidmodel_expression;
    qi::rule<Iterator, solidmodel(), locals<SolidModel::Ptr>, Skipper> r_solidmodel_subshape;
    qi::rule<Iterator, solidmodel(), locals<Model::Ptr>, Skipper> r_submodel_modelstep;
    
};


struct ModelStepsWriter
//: public std::map<std::string, T>
{
    void operator() (std::string s, SolidModel::Ptr ct);
};

template <typename Parser, typename Iterator>
bool parseISCADModel(Iterator first, Iterator last, Model::Ptr& model)
{
  Parser parser(model);
  skip_grammar<Iterator> skip;
  
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

void ModelStepsWriter::operator() (std::string s, SolidModel::Ptr ct)
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
  
  ISCADParser<std::string::iterator> parser(m);
  skip_grammar<std::string::iterator> skip;
  
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