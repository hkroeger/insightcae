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

// #define BOOST_SPIRIT_DEBUG

#include "latextools.h"

#include <string>

#include "base/boost_include.h"
#include "base/exception.h"
#include "boost/algorithm/string.hpp"
#include "base/tools.h"


#ifndef Q_MOC_RUN
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#endif

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace boost;

namespace insight
{

std::string cleanSymbols(const std::string& s)
{
  std::string result(s);
  boost::replace_all(result, "_", "\\_");
  boost::replace_all(result, "#", "\\#");
  boost::replace_all(result, "^", "\\textasciicircum");
  boost::replace_all(result, "[", "{[}");
  boost::replace_all(result, "]", "{]}");
  return result;
}




boost::filesystem::path cleanLatexImageFileName(const boost::filesystem::path& s)
{
  std::string stem=s.stem().string();
  replace_all(stem, ".", "_");
  return s.parent_path()/(stem+s.extension().string());
}


boost::filesystem::path findSharedImageFile(const std::string& file)
{
  insight::SharedPathList& spl = insight::SharedPathList::searchPathList;
  boost::filesystem::path p = file;
  try 
  { 
    p=spl.getSharedFilePath(file+".png"); 
  } 
  catch(...) 
  {
    try 
    { 
      p=spl.getSharedFilePath(file+".jpg"); 
    } 
    catch(...) 
    {
      try 
      { 
	p=spl.getSharedFilePath(file+".pdf"); 
      } 
      catch(...) 
      {
	p=file;
      } 
    } 
  }
  return p;
}


struct Replacements
{
  std::string reformatted_;
  std::string specialchars_;
  qi::symbols<char, std::string> simple_replacements_;
  
  Replacements()
  {
    specialchars_="$\\";
  }
  virtual ~Replacements() {};
  virtual void appendText(const std::string& text)
  {
    reformatted_ += text;
  }
  
  virtual void appendImage(double width, const std::string& imagename) =0;
  virtual void appendInlineFormula(const std::string& latex_formula) =0;
  virtual void appendDisplayFormula(const std::string& latex_formula) =0;
  
};

struct LaTeXReplacements
: Replacements
{
  LaTeXReplacements()
  {
    specialchars_+="\n_#[]^%&~<>";
    simple_replacements_.add("\n", "\\\\\n");
    simple_replacements_.add("_", "\\_");
    simple_replacements_.add("#", "\\#");
    simple_replacements_.add("&", "\\&");
    simple_replacements_.add("%", "\\%");
    simple_replacements_.add("[", "{[}");
    simple_replacements_.add("]", "{]}");
    simple_replacements_.add("~", "\\textasciitilde ");
    simple_replacements_.add("^", "\\textasciicircum ");
    simple_replacements_.add("<", "\\textless ");
    simple_replacements_.add(">", "\\textgreater ");
  }

  virtual void appendImage(double width, const std::string& imagename)
  {
    boost::filesystem::path fname = findSharedImageFile(imagename);
    fname = boost::filesystem::change_extension(fname, "");
    reformatted_ += str(format("\\includegraphics[keepaspectratio,width=%d\\linewidth]{%s}") % width % fname.string() );
  }

  virtual void appendInlineFormula(const std::string& latex_formula)
  {
    reformatted_ += "$"+latex_formula+"$";
  }
  virtual void appendDisplayFormula(const std::string& latex_formula)
  {
    reformatted_ += "$$"+latex_formula+"$$";
  }

};

struct HTMLReplacements
: Replacements
{
  HTMLReplacements()
  {
    specialchars_+="\n&<>^~";
    simple_replacements_.add("\n", "<br>\n");
    simple_replacements_.add("&", "&amp;");
    simple_replacements_.add("<", "&lt;");
    simple_replacements_.add(">", "&gt;");
    simple_replacements_.add("~", "&circ;");
    simple_replacements_.add("^", "&tilde;");
  }
  
  virtual void appendImage(double width, const std::string& imagename)
  {
    boost::filesystem::path fname = findSharedImageFile(imagename);
    
    reformatted_ += str(format("<img width=\"%g%%\" src=\"file://%s\">") % (100.*width) % fname.string() );
  }
  
  virtual void appendInlineFormula(const std::string& latex_formula)
  {
    reformatted_ += latex_formula;
  }

  virtual void appendDisplayFormula(const std::string& latex_formula)
  {
    reformatted_ += "<br>\n  "+latex_formula+"<br>\n";
  }

};

struct StringParser
  : qi::grammar<std::string::iterator >
{
  
  qi::rule< std::string::iterator > start;   
  qi::rule< std::string::iterator, std::string() > inlineformula, displayformula, image; 
    

    StringParser(const Replacements& rep)
    : qi::grammar<std::string::iterator>::base_type(start)
    {
      inlineformula =  qi::as_string[ '$' >> +(~qi::char_("$")) - qi::char_('$')  >> '$' ];
      displayformula =  qi::as_string[ qi::lit("$$") >> +(~qi::char_("$")) - qi::char_('$')  >> qi::lit("$$") ];
      
      image = 
      (
	qi::lit("\\includegraphics") 
	>> '[' >> qi::lit("width") >> *qi::char_(' ') >> '=' >> *qi::char_(' ') >> (qi::double_|qi::attr(1.0)) >> qi::lit("\\linewidth") >> ']'
	>> '{' >> qi::as_string[qi::lexeme[ *(qi::char_ - qi::char_('}')) ]] >> '}'
      ) [ phx::bind(&Replacements::appendImage, &rep, qi::_3, qi::_4) ]
	;
      
      start = *(
	qi::as_string[ +(~qi::char_(rep.specialchars_)) - qi::char_(rep.specialchars_) ] //[ std::cout<<"TEXT:{{"<<qi::_1<<"}}"<<std::endl ] 
	  [ phx::bind(&Replacements::appendText, &rep, qi::_1) ]
	|
	image
	|
	inlineformula 
	  [ phx::bind(&Replacements::appendInlineFormula, &rep, qi::_1) ]
	|
	displayformula 
	  [ phx::bind(&Replacements::appendDisplayFormula, &rep, qi::_1) ]
	| rep.simple_replacements_ [ phx::bind(&Replacements::appendText, &rep, qi::_1) ]
      )
      ; 
      
//       text = "Text{" >> content >> "};";  
//       escChar = '\\' >> qi::char_("\\{}");
      
//       BOOST_SPIRIT_DEBUG_NODE(formula);
//       BOOST_SPIRIT_DEBUG_NODE(image);
//       BOOST_SPIRIT_DEBUG_NODE(start);
      
          qi::on_error<qi::fail>(start,
                   phx::ref(std::cout)
                   << "Error! Expecting "
                   << qi::_4
                   << " here: '"
                   << phx::construct<std::string>(qi::_3, qi::_2)
                   << "'\n"
                  );
    }
};



std::string reformat(const std::string& simplelatex, const Replacements& rep)
{
  std::string original(simplelatex);
  
  StringParser parser(rep);
  
  std::string::iterator 
    start=original.begin(), 
    end=original.end();
    
  bool r = qi::parse(
      start,
      end,
      parser
  );
  
  if (start != end)
  {
    std::cout << "Fail at "<<int(start-original.begin())<<std::endl;
    throw insight::Exception("Failed to translate simple latex text: \""+simplelatex+"\"");
  }
  
  return rep.reformatted_;
}


SimpleLatex::SimpleLatex()
{}




SimpleLatex::SimpleLatex(const std::string& simpleLatex)
: simpleLatex_code_(simpleLatex)
{}



const std::string& SimpleLatex::simpleLatex() const
{
  return simpleLatex_code_;
}



std::string& SimpleLatex::simpleLatex()
{
  return simpleLatex_code_;
}




std::string SimpleLatex::toLaTeX() const
{
  LaTeXReplacements rep;
  return reformat(simpleLatex_code_, rep);
}




std::string SimpleLatex::toHTML(double maxWidth) const
{
  HTMLReplacements rep;
  return reformat(simpleLatex_code_, rep);
}




std::string SimpleLatex::toPlainText() const
{
  return simpleLatex_code_;
}



}
