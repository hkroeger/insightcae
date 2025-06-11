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
#include "base/casedirectory.h"
#include "boost/process.hpp"


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
  replace_all(stem, " ", "_");
  return s.parent_path()/(stem+s.extension().string());
}


boost::filesystem::path findSharedImageFile(const std::string& file)
{
    auto spl = SharedPathList::global();
    boost::filesystem::path p = file;
    try
    {
        p=spl.getSharedFilePath(file+".png");
    }
    catch(...)
    {
        try
        {
            p=spl.getSharedFilePath(file+".svg");
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
    }
    return p;
}





struct Replacements
{
  std::string reformatted_;
  std::string specialchars_;
  qi::symbols<char, std::string> simple_replacements_;

  enum Format {
    BOLD
  };
  
  Replacements()
  {
    specialchars_="$\\";
  }
  virtual ~Replacements();
  virtual void appendText(const std::string& text)
  {
    reformatted_ += text;
  }
  
  virtual void appendImage(double width, const std::string& imagename) =0;
  virtual void appendSvgImage(double width, const std::string& imagename) =0;
  virtual void appendInlineFormula(const std::string& latex_formula) =0;
  virtual void appendDisplayFormula(const std::string& latex_formula) =0;
  virtual void appendFormattedText(const std::string& text, Format fmt) =0;
  virtual void appendFootnote(const std::string& footnotetext) =0;

};

Replacements::~Replacements()
{}




struct PlainTextReplacements
: Replacements
{

  PlainTextReplacements();

  void appendImage(double, const std::string& imagename) override
  {
    boost::filesystem::path fname = findSharedImageFile(imagename);

    reformatted_ += str(format("\nfile://%s\n") % fname.string() );
  }

  void appendSvgImage(double, const std::string& imagename) override
  {
      return appendImage(0., imagename);
  }

  void appendInlineFormula(const std::string& latex_formula) override
  {
    reformatted_ += latex_formula;
  }

  void appendDisplayFormula(const std::string& latex_formula) override
  {
    reformatted_ += "\n  "+latex_formula+"\n";
  }

  void appendFormattedText(const std::string& text, Format) override
  {
    reformatted_ += text;
  }

  void appendFootnote(const std::string& footnotetext) override
  {
      reformatted_ += " (" + footnotetext + ")";
  }
};

PlainTextReplacements::PlainTextReplacements()
{}


struct LaTeXReplacements
: Replacements
{

  LaTeXReplacements();

  void appendImage(double width, const std::string& imagename) override
  {
    boost::filesystem::path fname = findSharedImageFile(imagename);
    fname = boost::filesystem::change_extension(fname, "");
    reformatted_ += str(format("\\includegraphics[keepaspectratio,width=%d\\linewidth]{%s}") % width % fname.string() );
  }

  void appendSvgImage(double width, const std::string& imagename) override
  {
      boost::filesystem::path fname = findSharedImageFile(imagename);
      fname = boost::filesystem::change_extension(fname, "");
      reformatted_ += str(format("\\includesvg[width=%d\\linewidth]{%s}") % width % fname.string() );
  }

  void appendInlineFormula(const std::string& latex_formula) override
  {
    reformatted_ += "$"+latex_formula+"$";
  }
  void appendDisplayFormula(const std::string& latex_formula) override
  {
    reformatted_ += "$$"+latex_formula+"$$";
  }
  void appendFormattedText(const std::string& text, Format) override
  {
    reformatted_ += "{\bf "+text+"}";
  }
  void appendFootnote(const std::string& footnotetext) override
  {
      reformatted_ += "\\footnote{" + footnotetext + "}";
  }

};

LaTeXReplacements::LaTeXReplacements()
{
  specialchars_+="\n_#[]^%&~<>";
  simple_replacements_.add("\n", "\\\\\n");
  simple_replacements_.add("_", "\\_");
  simple_replacements_.add("#", "\\#");
  simple_replacements_.add("&", "\\&");
  simple_replacements_.add("\\%", "\\%");
  simple_replacements_.add("%", "\\%");
  simple_replacements_.add("[", "{[}");
  simple_replacements_.add("]", "{]}");
  simple_replacements_.add("~", "\\textasciitilde ");
  simple_replacements_.add("^", "\\textasciicircum ");
  simple_replacements_.add("<", "\\textless ");
  simple_replacements_.add(">", "\\textgreater ");
}

struct TemporaryCachedFile
: public boost::filesystem::path
{
  TemporaryCachedFile(const std::string& extension)
    : boost::filesystem::path
      (
        boost::filesystem::unique_path
        (
          boost::filesystem::temp_directory_path()
          / ("html-replacement-%%%%%%%."+extension)
        )
       )
  {
  }

  ~TemporaryCachedFile()
  {
    if (boost::filesystem::exists(*this))
      boost::filesystem::remove(*this);
  }
};





typedef std::shared_ptr<TemporaryCachedFile> TemporaryCachedFilePtr;

// maybe need to add some expiry duration...
struct FilesCache
    : std::map<std::string, TemporaryCachedFilePtr>
{
};

void runLatex(const std::string& formula_code, const boost::filesystem::path& output)
{
  insight::CurrentExceptionContext ex("rendering formula into PNG image");

  insight::dbg()<<"formula code: "<<formula_code<<std::endl;

  CaseDirectory subdir(false, boost::filesystem::temp_directory_path()/"runLatex" );
  insight::dbg()<<"subdir: "<<subdir<<std::endl;

  boost::filesystem::path tex_filename = subdir/"input.tex";
  std::ofstream tex( tex_filename.string() );
  tex<<
        "\\documentclass{article}\n"
        "\\pagestyle{empty}\n"
        "\\begin{document}\n"
        "\\[" << formula_code << "\\]\n"
        "\\newpage\n"
        "\\end{document}"
        ;
  tex.close();


  boost::process::system
  (
     boost::process::search_path("latex"),
     boost::process::args
        ({
          "-file-line-error-style",
          "-interaction=nonstopmode",
          tex_filename.filename().string()
         }),
     boost::process::start_dir(boost::filesystem::absolute(subdir))
  );

  boost::process::system
  (
     boost::process::search_path("dvipng"),
     boost::process::args
        ({"--freetype0",
          "-Q", "9",
          "-z", "3",
          "--depth",
          "-q",
          "-T", "tight",
          "-D", "150",
          "-bg", "Transparent",
          "-o", output.string(),
          tex_filename.replace_extension(".dvi").string()
         })
  );

  insight::dbg()<<"tex file: "<<tex_filename<<", output file: "<<output<<std::endl;
}

struct FormulaRenderFilesCache
: public FilesCache
{
  boost::filesystem::path renderLatexFormula(const std::string& formula_code)
  {
    auto i=find(formula_code);
    if (i!=end())
    {
      return *i->second;
    }
    else
    {
      TemporaryCachedFilePtr result(new TemporaryCachedFile("png"));

      runLatex(formula_code, *result);

      insert(value_type(formula_code, result));
      return *result;
    }
  }
};




struct HTMLReplacements
: Replacements
{
  static FormulaRenderFilesCache formulaCache;

  int imageWidth_;

  HTMLReplacements(int imageWidth);
  
  void appendImage(double width, const std::string& imagename) override
  {
    boost::filesystem::path fname = findSharedImageFile(imagename);
    std::string code =
        str(format("<img width=\"%d\" src=\"file:///%s\">")
            % int( double(imageWidth_)*width ) % fname.generic_path().string() );
    insight::dbg()<<code<<std::endl;
    reformatted_ += code;
  }

  void appendSvgImage(double width, const std::string& imagename) override
  {
      return appendImage(width, imagename);
  }

  void appendInlineFormula(const std::string& latex_formula) override
  {
    auto rff = formulaCache.renderLatexFormula(latex_formula);
    std::string code = "<img src=\"file:///"+rff.generic_path().string()+"\">";
    insight::dbg()<<code<<std::endl;
    reformatted_ += code;
  }

  void appendDisplayFormula(const std::string& latex_formula) override
  {
    auto rff = formulaCache.renderLatexFormula(latex_formula);
    std::string code = "<br>\n  <img src=\"file:///"+rff.generic_path().string()+"\"><br>\n";
    insight::dbg()<<code<<std::endl;
    reformatted_ += code;
  }

  void appendFormattedText(const std::string& text, Format) override
  {
    reformatted_ += "<b>"+text+"</b>";
  }

  void appendFootnote(const std::string& footnotetext) override
  {
      reformatted_ += " ("+footnotetext+")";
  }
};

FormulaRenderFilesCache HTMLReplacements::formulaCache;

std::string combine(const std::vector<std::string>& strings)
{
  return join(strings, "");
}

HTMLReplacements::HTMLReplacements(int imageWidth)
  : imageWidth_(imageWidth)
{
  specialchars_+="\n&<>^~";
  simple_replacements_.add("\n", "<br>\n");
  simple_replacements_.add("&", "&amp;");
  simple_replacements_.add("<", "&lt;");
  simple_replacements_.add(">", "&gt;");
  simple_replacements_.add("^", "&circ;");
  simple_replacements_.add("~", "&tilde;");
}

struct StringParser
  : qi::grammar<std::string::iterator >
{
  
  qi::rule< std::string::iterator > start;   
  qi::rule< std::string::iterator, std::string() >
   inlineformula, displayformula, image, svgimage, verbatimtext;
    

    StringParser(Replacements& rep)
    : qi::grammar<std::string::iterator>::base_type(start)
    {
      inlineformula =  qi::as_string[ '$' > +(~qi::char_("$")) - qi::char_('$')  > '$' ];
      displayformula =  qi::as_string[ qi::lit("$$") > +(~qi::char_("$")) - qi::char_('$') > qi::lit("$$") ];
      verbatimtext =
            qi::lit("\\begin{verbatim}")
          > qi::lexeme[ *( (qi::as_string[~qi::char_(rep.specialchars_)])|rep.simple_replacements_ - qi::lit("\\end{verbatim}") ) ][ qi::_val = phx::bind(&combine, qi::_1) ]
          > qi::lit("\\end{verbatim}");

      image = 
      (
	qi::lit("\\includegraphics") 
        > '[' > qi::lit("width") > '=' > (qi::double_|qi::attr(1.0)) > qi::lit("\\linewidth") > ']'
        > '{' > qi::as_string[qi::lexeme[ *(qi::char_ - qi::char_('}')) ]] > '}'
      ) [ phx::bind(&Replacements::appendImage, &rep, qi::_1, qi::_2) ]
	;

      svgimage =
          (
              qi::lit("\\includesvg")
              > '[' > qi::lit("width") > '=' > (qi::double_|qi::attr(1.0)) > qi::lit("\\linewidth") > ']'
              > '{' > qi::as_string[qi::lexeme[ *(qi::char_ - qi::char_('}')) ]] > '}'
              ) [ phx::bind(&Replacements::appendSvgImage, &rep, qi::_1, qi::_2) ]
          ;

      start = *(
        ( qi::lit("\\footnote")
           > '{'
           > qi::as_string[ *(qi::char_ - qi::char_('}')) ]
                          [ phx::bind(&Replacements::appendFootnote, &rep, qi::_1) ]
           > '}' )
        |
        verbatimtext
          [ phx::bind(&Replacements::appendText, &rep, qi::_1) ]
        |
        ( '{' >>
          ( qi::as_string[ qi::lit("\\bf") >> qi::lexeme[ *(qi::char_ - qi::char_('}')) ] >> '}' ] )
            [ phx::bind(&Replacements::appendFormattedText, &rep, qi::_1, Replacements::BOLD) ]
//          |
//          ( qi::as_string[ qi::lit("\\sl") >> qi::lexeme[ *(qi::char_ - qi::char_('}')) ] >>qi::lit("}") ] )
//            [ phx::bind(&Replacements::appendFormattedText, &rep, qi::_1, Replacements::BOLD) ]
        )
        |
        qi::as_string[ +(~qi::char_(rep.specialchars_)) - qi::char_(rep.specialchars_) ]
          [ phx::bind(&Replacements::appendText, &rep, qi::_1) ]
        |
        image
        |
        svgimage
        |
        displayformula
          [ phx::bind(&Replacements::appendDisplayFormula, &rep, qi::_1) ]
        |
        inlineformula
          [ phx::bind(&Replacements::appendInlineFormula, &rep, qi::_1) ]
            |
            rep.simple_replacements_ [ phx::bind(&Replacements::appendText, &rep, qi::_1) ]
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



std::string reformat(const std::string& simplelatex, Replacements& rep)
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


SimpleLatex::SimpleLatex(const SimpleLatex& slt)
: simpleLatex_code_(slt.simpleLatex_code_)
{}


SimpleLatex::SimpleLatex(const std::string &slt)
    : simpleLatex_code_(slt)
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




std::string SimpleLatex::toHTML(int imageWidth) const
{
  HTMLReplacements rep(imageWidth);
  return reformat(simpleLatex_code_, rep);
}



std::string SimpleLatex::toPlainText() const
{
  PlainTextReplacements rep;
  return reformat(simpleLatex_code_, rep);
}

bool SimpleLatex::empty() const
{
  return simpleLatex_code_.empty();
}



std::string SimpleLatex::LaTeXFromPlainText(const std::string&)
{
    throw insight::Exception("SimpleLatex::LaTeXFromPlainText: Not Implemented");
}




bool SimpleLatex::operator!=(const SimpleLatex &o) const
{
    return simpleLatex_code_!=o.simpleLatex_code_;
}




}
