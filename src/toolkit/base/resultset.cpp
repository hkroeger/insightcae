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


#include "resultset.h"

#include "base/latextools.h"
#include "base/tools.h"
#include "base/case.h"
#include "base/analysis.h"
#include "base/parameters/subsetparameter.h"

#include <fstream>
#include <algorithm>

#include "base/boost_include.h"

#include "rapidxml/rapidxml_print.hpp"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
namespace fs=boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;

namespace insight
{



defineType ( ResultSet );
addToFactoryTable ( ResultElement, ResultSet );



ResultSet::ResultSet(const std::string& analysisName)
  : ResultElement ( "", "", "" )
{
  if (analysisName!="") p_=Analysis::defaultParameters(analysisName);
}

ResultSetPtr ResultSet::createFromFile( const boost::filesystem::path& fileName, const std::string& analysisName )
{
    auto r = std::make_shared<ResultSet>(analysisName);
    r->readFromFile(fileName);
    return r;
}

ResultSetPtr ResultSet::createFromStream( std::istream& is, const std::string& analysisName )
{
    auto r = std::make_shared<ResultSet>(analysisName);
    r->readFromStream(is);
    return r;
}

ResultSetPtr ResultSet::createFromString( const std::string& cont, const std::string& analysisName )
{
    auto r = std::make_shared<ResultSet>(analysisName);
    r->readFromString(cont);
    return r;
}


ResultSet::ResultSet
(
    const ParameterSet& p,
    const std::string& title,
    const std::string& subtitle,
    const std::string* date,
    const std::string* author
)
    : ResultElement ( "", "", "" ),
      p_ ( p ),
      title_ ( title ),
      subtitle_ ( subtitle ),
      introduction_()
{
    if ( date ) {
        date_=*date;
    } else {
        using namespace boost::gregorian;
        date_=to_iso_extended_string ( day_clock::local_day() );
    }

    if ( author ) {
        author_=*author;
    } else {
        char  *iu=getenv ( "INSIGHT_REPORT_AUTHOR" );
        if ( iu ) {
            author_=iu;
        } else {
            char* iua=getenv ( "USER" );
            if ( iua ) {
                author_=iua;
            } else {
                author_="";
            }
        }
    }
}

ResultSet::ResultSet ( const std::string& shortdesc, const std::string& longdesc, const std::string& )
  : ResultSet( ParameterSet(), shortdesc, longdesc )
{}

void ResultSet::readFromNode ( const std::string&, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node )
{
  ResultElementCollection::readElementsFromNode(doc, node);
}


ResultSet::~ResultSet()
{}


ResultSet::ResultSet ( const ResultSet& other )
    : //ptr_map< std::string, ResultElement>(other),
    ResultElementCollection ( other ),
    ResultElement ( "", "", "" ),
    p_ ( other.p_ ),
    title_ ( other.title_ ),
    subtitle_ ( other.subtitle_ ),
    date_ ( other.date_ ),
    author_ ( other.author_ ),
    introduction_ ( other.introduction_ )
{
}


void ResultSet::transfer ( const ResultSet& other )
{
//   ptr_map< std::string, ResultElement>::operator=(other);
    std::map< std::string, ResultElementPtr>::operator= ( other );
    p_=other.p_;
    title_=other.title_;
    subtitle_=other.subtitle_;
    author_=other.author_;
    date_=other.date_;
    introduction_=other.introduction_;
}


void ResultSet::writeLatexHeaderCode ( std::ostream& f ) const
{
    for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
        i->second->writeLatexHeaderCode ( f );
    }
}


void ResultSet::writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const
{
    if ( level>0 ) {
        f << title_ << "\n\n";

        f << subtitle_ << "\n\n";
    }

    if ( !introduction_.empty() ) {
        f << latex_subsection ( level ) << "{Introduction}\n";

        f<<introduction_;
    }

    if ( p_.size() >0 ) {
        f << latex_subsection ( level ) << "{Input Parameters}\n";

        f<<p_.latexRepresentation();
    }

    f << latex_subsection ( level ) << "{Numerical Result Summary}\n";

    writeLatexCodeOfElements ( f, name, level, outputfilepath );

//   for (ResultSet::const_iterator i=begin(); i!=end(); i++)
//   {
//     f << latex_subsection(level+1) << "{" << cleanSymbols(i->first) << "}\n";
//     f << cleanSymbols(i->second->shortDescription()) << "\n\n";
//
//     std::string subelemname=i->first;
//     if (name!="")
//       subelemname=name+"__"+i->first;
//
//     i->second->writeLatexCode(f, subelemname, level+2, outputfilepath);
//
//     f << "\n\n" << cleanSymbols(i->second->longDescription()) << "\n\n";
//     f << endl;
//   }
}


xml_node< char >* ResultSet::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    ResultElementCollection::appendElementsToNode ( doc, *child );

    return child;
}


void ResultSet::exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const
{
    path outsubdir ( outputdirectory/name );
    create_directory ( outsubdir );
    for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
        i->second->exportDataToFile ( i->first, outsubdir );
    }
}


void ResultSet::readFromFile ( const boost::filesystem::path& file )
{
  CurrentExceptionContext ex("reading results set from file "+file.string());
  std::string contents;
  readFileIntoString(file, contents);
  readFromString(contents);
}

void ResultSet::readFromStream ( std::istream& is )
{
  CurrentExceptionContext ex("reading result set from input stream");

  std::string contents;
  readStreamIntoString(is, contents);
  readFromString(contents);
}

void ResultSet::readFromString ( const std::string& contents )
{
  CurrentExceptionContext ex("reading result set from content string");


  char* startChar = const_cast<char*>(&contents[0]);
  xml_document<> doc;
  doc.parse<0> ( startChar );

  xml_node<> *rootnode = doc.first_node ( "root" );

  auto *pn = rootnode->first_node("parameters");
  p_.readFromNode( doc, *pn, "/" );

  auto *rn = rootnode->first_node("results");
  title_=rn->first_attribute("title")->value();
  subtitle_=rn->first_attribute("subtitle")->value();
  date_=rn->first_attribute("date")->value();
  author_=rn->first_attribute("author")->value();
  introduction_=rn->first_attribute("introduction")->value();
  ResultElementCollection::readElementsFromNode ( doc, *rn );
}




void ResultSet::saveToFile ( const boost::filesystem::path& file ) const
{
  std::ofstream f ( file.c_str() );
  saveToStream(f);
}

void ResultSet::saveToStream(ostream &os) const
{
  xml_document<> doc;

  // xml declaration
  xml_node<>* decl = doc.allocate_node ( node_declaration );
  decl->append_attribute ( doc.allocate_attribute ( "version", "1.0" ) );
  decl->append_attribute ( doc.allocate_attribute ( "encoding", "utf-8" ) );
  doc.append_node ( decl );

  xml_node<> *rootnode = doc.allocate_node ( node_element, "root" );
  doc.append_node ( rootnode );

  xml_node<>* pc = doc.allocate_node ( node_element, doc.allocate_string ( "parameters" ) );
  p_.appendToNode(doc, *pc, "/");
  rootnode->append_node ( pc );

  xml_node<>* rc = doc.allocate_node ( node_element, doc.allocate_string ( "results" ) );
  rc->append_attribute(
        doc.allocate_attribute(
          "title",
          doc.allocate_string ( title_.c_str() )
          )
        );
  rc->append_attribute(
        doc.allocate_attribute(
          "subtitle",
          doc.allocate_string ( subtitle_.c_str() )
          )
        );
  rc->append_attribute(
        doc.allocate_attribute(
          "date",
          doc.allocate_string ( date_.c_str() )
          )
        );
  rc->append_attribute(
        doc.allocate_attribute(
          "author",
          doc.allocate_string ( author_.c_str() )
          )
        );
  rc->append_attribute(
        doc.allocate_attribute(
          "introduction",
          doc.allocate_string ( introduction_.c_str() )
          )
        );
  ResultElementCollection::appendElementsToNode ( doc, *rc );
  rootnode->append_node ( rc );

  os << doc << std::endl;
}




void ResultSet::saveAs(const boost::filesystem::path &outfile) const
{
    auto ext=boost::algorithm::to_lower_copy(outfile.extension().string());
    if (ext==".isr")
    {
        saveToFile(outfile);
    }
    else if (ext==".tex")
    {
        writeLatexFile(outfile);
    }
    else if (ext==".pdf")
    {
        generatePDF(outfile);
    }
    else
    {
        throw insight::Exception("unrecognized file type: \""+outfile.string()+"\"");
    }
}




std::string builtin_template=
    "\\documentclass[a4paper,10pt]{scrartcl}\n"
    "\\usepackage{hyperref}\n"
    "\\usepackage{fancyhdr}\n"
    "\\pagestyle{fancy}\n"
    "###HEADER###\n"
    "\\begin{document}\n"
    "\\title{###TITLE###\\\\\n"
    "\\vspace{0.5cm}\\normalsize{###SUBTITLE###}}\n"
    "\\date{###DATE###}\n"
    "\\author{###AUTHOR###}\n"
    "\\maketitle\n"
    "\\tableofcontents\n"
    "###CONTENT###\n"
    "\\end{document}\n";


void ResultSet::writeLatexFile ( const boost::filesystem::path& file ) const
{
  CurrentExceptionContext ec("writing latex representation of result set into file "+file.string());

    path filepath ( absolute ( file ) );

    std::ostringstream header, content;

    header<<"\\newcommand{\\PlotFrameB}[2]{%\n"
          <<"\\includegraphics[#1]{#2}\\endgroup}\n"
          <<"\\def\\PlotFrame{\\begingroup\n"
          <<"\\catcode`\\_=12\n"
          <<"\\PlotFrameB}\n"

          <<"\\usepackage{enumitem}\n"
          "\\setlist[enumerate]{label*=\\arabic*.}\n"
          "\\renewlist{enumerate}{enumerate}{10}\n"

          ;
    writeLatexHeaderCode ( header );

    writeLatexCode ( content, "", 0, filepath.parent_path() );

    std::unique_ptr<TemplateFile> reportTemplate;

    std::string envvarname="INSIGHT_REPORT_TEMPLATE";
    if ( char *TEMPL=getenv ( envvarname.c_str() ) )
    {
      reportTemplate.reset(new TemplateFile(fs::path(TEMPL)));
    }
    else
    {
      reportTemplate.reset(new TemplateFile(builtin_template));
    }

    reportTemplate->replace("AUTHOR", author_ );
    reportTemplate->replace("DATE", date_ );
    reportTemplate->replace("TITLE", title_ );
    reportTemplate->replace("SUBTITLE", subtitle_ );

    reportTemplate->replace("HEADER", header.str() );
    reportTemplate->replace("CONTENT", content.str() );

    reportTemplate->write(filepath);

    {
        path outdir ( filepath.parent_path() / ( "report_data_"+filepath.stem().string() ) );
        create_directory ( outdir );
        for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
            i->second->exportDataToFile ( i->first, outdir );
        }
    }
}

void ResultSet::generatePDF ( const boost::filesystem::path& file ) const
{
  std::string stem = file.filename().stem().string();

  {
      path outdir ( file.parent_path() / ( "report_data_"+stem ) );
      create_directory ( outdir );
      for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
          i->second->exportDataToFile ( i->first, outdir );
      }
  }

  CaseDirectory gendir(false);
  boost::filesystem::path outpath = gendir / (stem+".tex");
  writeLatexFile( outpath );

  for (int i=0; i<2; i++)
  {
      if ( ::system( str( format("cd \"%s\" && pdflatex -interaction=batchmode \"%s\"") % gendir.string() % outpath.filename().string() ).c_str() ))
      {
          throw insight::Exception("TeX input file was written but could not execute pdflatex successfully.");
      }
  }

  boost::filesystem::copy_file( gendir/ (stem+".pdf"), file, copy_option::overwrite_if_exists );

}



ParameterSetPtr ResultSet::convertIntoParameterSet() const
{
    ParameterSetPtr ps ( new ParameterSet() );
    for ( const_iterator::value_type rp: *this ) {
        ParameterPtr p=rp.second->convertIntoParameter();
        if ( p )
        {
            ps->insert ( ParameterSet::value_type(rp.first, std::unique_ptr<Parameter>(p->clone())) );
        }
    }
    return ps;
}


ParameterPtr ResultSet::convertIntoParameter() const
{
    auto ps = std::make_shared<SubsetParameter>();
    ps->subsetRef().extend ( convertIntoParameterSet()->entries() );
    return ps;
}







ResultElementPtr ResultSet::clone() const
{
    std::unique_ptr<ResultSet> nr ( new ResultSet ( p_, title_, subtitle_, &author_, &date_ ) );
    for ( ResultSet::const_iterator i=begin(); i!=end(); i++ ) {
//         cout<<i->first<<endl;
        std::string key ( i->first );
        nr->insert ( key, i->second->clone() );
    }
    nr->setOrder ( order() );
    nr->introduction() =introduction_;
    return ResultElementPtr ( nr.release() );
}


}



