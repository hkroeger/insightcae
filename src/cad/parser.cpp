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

namespace insight {
namespace cad {
  
namespace parser {  
  
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

scalar Model::lookupModelScalar(const std::string& modelname, const std::string& scalarname) const
{
  const Model::Ptr* pptr=modelSymbols.find(modelname);
  if (pptr)
  {
    scalar *sptr=(*pptr)->scalarSymbols.find(scalarname);
    if (sptr)
    {
      return *sptr;
    }
    else
      throw insight::Exception("scalar symbol "+scalarname+" not found in model "+modelname+"!");
  }
  else
    throw insight::Exception("model "+modelname+" not found!");
}

solidmodel Model::lookupModelModelstep(const std::string& modelname, const::string& modelstepname) const
{
  const Model::Ptr* pptr=modelSymbols.find(modelname);
  if (pptr)
  {
    solidmodel *sptr=(*pptr)->modelstepSymbols.find(modelstepname);
    if (sptr)
    {
      return *sptr;
    }
    else
      throw insight::Exception("model step "+modelstepname+" not found in model "+modelname+"!");
  }
  else
    throw insight::Exception("model "+modelname+" not found!");
}
  
Model::Ptr loadModel(const std::string& name)
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
    p=p/(name+".iscad");
    if (exists(p))
    {
      Model::Ptr model;
      if (parseISCADModelFile(p, model))
      {
	cout<<"Successfully parsed model "<<p<<endl;
	return model;
      }
    }
  }
  cout<<"Failed to parse model "<<name<<endl;
  return Model::Ptr();
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


template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct ISCADParser
  : qi::grammar<Iterator, Skipper>
{

  Model::Ptr model_;

    ISCADParser()
      : ISCADParser::base_type(r_model),
	model_(new Model)
    {
      
	using namespace qi;
	using namespace phx;
	using namespace insight::cad;
	
        r_model =  *( r_assignment | r_modelstep | r_loadmodel ) 
		  >> -( lit("@post")  >> *r_postproc);
	
	r_loadmodel = ( lit("loadmodel") >> '(' >> r_identifier >> ')' >> ';' ) 
	  [ phx::bind(model_->modelSymbols.add, _1, loadModel_(_1)) ];
	
	r_assignment = 
	  ( r_identifier >> '='  >> r_scalarExpression >> ';') [ phx::bind(model_->scalarSymbols.add, _1, _2) ]
	  |
	  ( r_identifier >> '='  >> r_vectorExpression >> ';') [ phx::bind(model_->vectorSymbols.add, _1, _2) ]
	  |
// 	  ( r_identifier >> '='  >> r_edgeFeaturesExpression >> ';') [ phx::bind(model_->edgeFeatureSymbols.add, _1, _2) ]
// 	  |
	  ( r_identifier >> '='  >> r_datumExpression >> ';') [ phx::bind(model_->datumSymbols.add, _1, _2) ]
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
	
        r_modelstep  =  ( r_identifier >> ':' > r_solidmodel_expression >> ';' ) [ phx::bind(model_->modelstepSymbols.add, _1, _2) ];
	
	
	r_solidmodel_expression =
	 r_solidmodel_term [_val=_1 ]
	 >> *( '-' >> r_solidmodel_term [_val=construct<solidmodel>(new_<BooleanSubtract>(*_val, *_1))] )
	 ;
	
	r_solidmodel_term =
	 r_solidmodel_primary [_val=_1 ]
	 >> *( '|' >> r_solidmodel_primary [_val=construct<solidmodel>(new_<BooleanUnion>(*_val, *_1))] )
	 ;
	
	r_solidmodel_primary = 
	 lexeme[ model_->modelstepSymbols >> !(alnum | '_') ] [ _val = _1 ]

	 | ( r_identifier >> '.' > r_identifier ) 
	    [ _val = phx::bind(&Model::lookupModelModelstep, *model_, _1, _2) ]
	 | ( '(' >> r_solidmodel_expression [_val=_1] >> ')' )
	 
         | ( lit("import") > '(' >> r_path >> ')' ) [ _val = construct<solidmodel>(new_<SolidModel>(_1)) ]
         | ( lit("Sketch") > '(' >> r_datumExpression >> ',' >> r_path >> ',' >> r_string >> ')' ) 
	    [ _val = construct<solidmodel>(new_<Sketch>(*_1, _2, _3)) ]
         
         | ( lit("CircularPattern") > '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> ',' 
	    >> r_vectorExpression >> ','>> r_scalarExpression >> ')' ) 
	     [ _val = construct<solidmodel>(new_<CircularPattern>(*_1, _2, _3, _4)) ]
         | ( lit("LinearPattern") > '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> ',' 
	    >> r_scalarExpression >> ')' ) 
	     [ _val = construct<solidmodel>(new_<LinearPattern>(*_1, _2, _3)) ]
         
         | ( lit("Transform") > '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> ',' 
	    >> r_vectorExpression >> ')' ) 
	     [ _val = construct<solidmodel>(new_<Transform>(*_1, _2, _3)) ]
         | ( lit("Compound") > '(' >> ( r_solidmodel_expression % ',' ) >> ')' ) 
	     [ _val = construct<solidmodel>(new_<Compound>(_1)) ]

	 // Primitives
	 | ( lit("Sphere") > '(' >> r_vectorExpression >> ',' >> r_scalarExpression >> ')' ) 
	      [ _val = construct<solidmodel>(new_<Sphere>(_1, _2)) ]
	 | ( lit("Cylinder") > '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_scalarExpression >> ')' ) 
	      [ _val = construct<solidmodel>(new_<Cylinder>(_1, _2, _3)) ]
	 | ( lit("Box") > '(' >> r_vectorExpression >> ',' >> r_vectorExpression 
			>> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> -(  ',' >> lit("centered") >> attr(true) ) >> ')' ) 
	      [ _val = construct<solidmodel>(new_<Box>(_1, _2, _3, _4, _5)) ]
// 	 | ( lit("Fillet") > '(' >> r_solidmodel_expression >> ',' >> r_edgeFeaturesExpression >> ',' >> r_scalarExpression >> ')' ) 
// 	      [ _val = construct<solidmodel>(new_<Fillet>(*_1, _2, _3)) ]
// 	 | ( lit("Chamfer") > '(' >> r_solidmodel_expression >> ',' >> r_edgeFeaturesExpression >> ',' >> r_scalarExpression >> ')' ) 
// 	      [ _val = construct<solidmodel>(new_<Chamfer>(*_1, _2, _3)) ]
	 | ( lit("Extrusion") > '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> -(  ',' >> lit("centered") >> attr(true) ) >> ')' ) 
	      [ _val = construct<solidmodel>(new_<Extrusion>(*_1, _2, _3)) ]
	 | ( lit("Revolution") > '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_scalarExpression >> -(  ',' >> lit("centered") >> attr(true) ) >> ')' ) 
	      [ _val = construct<solidmodel>(new_<Revolution>(*_1, _2, _3, _4, _5)) ]
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
    
};


struct ModelStepsWriter
//: public std::map<std::string, T>
{
    void operator() (std::string s, SolidModel::Ptr ct);
};

template <typename Parser, typename Iterator>
bool parseISCADModel(Iterator first, Iterator last, Model::Ptr& model)
{
  Parser parser;
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
  
  model = parser.model_;
  
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
  
  ISCADParser<std::string::iterator> parser;
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
  
  m = parser.model_;
  
  return r;
}

}
}