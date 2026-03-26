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
#include "base/rapidxml.h"
#include "base/resultreporttemplates.h"

#include "base/latextools.h"
#include "base/tools.h"
#include "base/case.h"
#include "base/analysis.h"
#include "base/parameters/subsetparameter.h"

#include <fstream>
#include <algorithm>
#include <memory>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/date_time.hpp>
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
// addToFactoryTable ( ResultElement, ResultSet );




std::unique_ptr<insight::ResultSet>
ResultSet::createFromFile(
    const boost::filesystem::path& fileName,
    std::unique_ptr<ParameterSet> p )
{
    auto r = std::make_unique<ResultSet>(std::move(p));
    r->readFromFile(fileName);
    return r;
}

std::unique_ptr<insight::ResultSet>
ResultSet::createFromStream(
    std::istream& is,
    std::unique_ptr<ParameterSet> p )
{
    auto r = std::make_unique<ResultSet>(std::move(p));
    r->readFromStream(is);
    return r;
}

std::unique_ptr<insight::ResultSet>
ResultSet::createFromString(
    const std::string& cont,
    std::unique_ptr<ParameterSet> p )
{
    auto r = std::make_unique<ResultSet>(std::move(p));
    r->readFromString(cont);
    return r;
}


ResultSet::ResultSet
(
    std::unique_ptr<ParameterSet> p,
    const std::string& title,
    const std::string& subtitle,
    const std::string* date,
    const std::string* author
)
    : ResultElementCollection ( "", "", "" ),
      p_ ( std::move(p) ),
      title_ ( title ),
      subtitle_ ( subtitle ),
      introduction_()
{
    if (p_)
    {
        p_->setParent(this);
    }

    if ( date )
    {
        date_ = *date;
    }
    else
    {
        using namespace boost::gregorian;
        date_ = to_iso_extended_string ( day_clock::local_day() );
    }

    if ( author )
    {
        author_ = *author;
    }
    else
    {
        if ( char *iu = getenv("INSIGHT_REPORT_AUTHOR") )
        {
            author_ = iu;
        }
        else
        {
            if ( char* iua = getenv("USER") )
            {
                author_ = iua;
            }
            else
            {
                author_ = "";
            }
        }
    }
}



ResultSet::~ResultSet()
{}


// ResultSet::ResultSet ( const ResultSet& other )
//     : //ptr_map< std::string, ResultElement>(other),
//     ResultElementCollection ( other ),
//     p_ ( other.p_ ),
//     title_ ( other.title_ ),
//     subtitle_ ( other.subtitle_ ),
//     date_ ( other.date_ ),
//     author_ ( other.author_ ),
//     introduction_ ( other.introduction_ )
// {
// }


void ResultSet::transfer ( ResultSet& other )
{
//   ptr_map< std::string, ResultElement>::operator=(other);
    ResultElementCollection::transfer(other);
    if (other.p_)
    {
        p_=other.p_->cloneAs<ParameterSet>();
        p_->setParent(this);
    }
    title_=other.title_;
    subtitle_=other.subtitle_;
    author_=other.author_;
    date_=other.date_;
    introduction_=other.introduction_;
}

void ResultSet::clearInputParameters()
{
    p_.reset();
}


void ResultSet::insertLatexHeaderCode ( std::set<std::string>& hc ) const
{
    for ( auto& i: static_cast<const ResultElement&>(*this) )
    {
        i.insertLatexHeaderCode(hc);
    }
}


std::string ResultSet::latexRepresentation(
    const std::string& name,
    int level,
    const FileStorageInfo& fsi ) const
{
    std::ostringstream f;

    if ( level>0 )
    {
        f << title_ << "\n\n";

        f << subtitle_ << "\n\n";
    }

    if ( !introduction_.empty() )
    {
        f << latex_subsection ( level ) << "{Introduction}\n";

        f<<introduction_;
    }

    if ( p_ && p_->size()>0 ) {
        f << latex_subsection ( level ) << "{Input Parameters}\n";

        f << p_->latexRepresentation("inputParameters", level, fsi);
    }

    f << latex_subsection ( level ) << "{Numerical Result Summary}\n";

    f << ResultElementCollection::latexRepresentation ( name, level, fsi );

    return f.str();
}

boost::filesystem::path
ResultSet::reportDataPath(const boost::filesystem::path &outFileName)
{
    return
        outFileName.parent_path() /
        ( "report_data_"+outFileName.stem().string() )
        ;
}




void ResultSet::exportDataToFile (
    const std::string& name,
    const boost::filesystem::path& outputdirectory ) const
{
    auto outsubdir = outputdirectory/name;
    create_directory ( outsubdir );
    for ( auto& i: static_cast<const ResultElement&>(*this) )
    {
        if (auto *re=dynamic_cast<const ResultElement*>(&i))
            re->exportDataToFile ( re->name(), outsubdir );
    }
}








void ResultSet::saveAs(const boost::filesystem::path &outfile, ActionProgress *ap) const
{
    auto ext=boost::algorithm::to_lower_copy(outfile.extension().string());
    if (ext==".isr")
    {
        saveToFile(outfile);
    }
    else if (ext==".tex")
    {
        writeLatexFile(outfile, ap);
    }
    else if (ext==".pdf")
    {
        generatePDF(outfile, ap);
    }
    else
    {
        throw insight::Exception("unrecognized file type: \""+outfile.string()+"\"");
    }
}






void ResultSet::writeLatexFile (
    const boost::filesystem::path& file,
    ActionProgress* ap,
    const OutputProperties& outProps ) const
{
    if (ap) ap->setNSteps(3 + static_cast<const ResultElement&>(*this).nChildren());

  CurrentExceptionContext ec(
              "writing latex representation of result set into file "
              + file.string() );

    if (ap) ap->stepUp("Creating file");
    auto filepath = boost::filesystem::absolute ( file );

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

    std::set<std::string> headerCode;
    if (ap) ap->addSteps(headerCode.size());
    insertLatexHeaderCode ( headerCode );
    for (const auto& hc: headerCode)
    {
        if (ap) ap->stepUp("Creating content buffer");
        header << hc << std::endl;
    }

    auto reportData = reportDataPath(filepath);
    create_directory ( reportData );

    FileStorageInfo fsi(filepath.parent_path(), reportData);
    fsi.elementFilter=outProps.filter;
    content << latexRepresentation (
        "", 0, fsi );


    if (ap) ap->stepUp("Inserting content into template");

    auto &reportTemplate =
            ResultReportTemplates::globalInstance().defaultItem();

    auto reportInput = std::make_unique<TemplateFile>(
                static_cast<const std::string&>(reportTemplate) );


    reportInput->replace("AUTHOR", author_ );
    reportInput->replace("DATE", date_ );
    reportInput->replace("TITLE", title_ );
    reportInput->replace("SUBTITLE", subtitle_ );

    reportInput->replace("HEADER", header.str() );
    reportInput->replace("CONTENT", content.str() );


    reportInput->write(filepath);
    reportTemplate.writeAdditionalFiles(filepath.parent_path());

    for ( auto& i: static_cast<const ResultElement&>(*this) )
    {
        if (auto *re=dynamic_cast<const ResultElement*>(&i))
        {
            if (ap) ap->stepUp(boost::str(boost::format("Adding %s to report")%re->name()));
            re->exportDataToFile ( re->name(), reportData );
        }
    }

    if (ap) ap->stepUp("Finished");

}




void ResultSet::generatePDF (
    const boost::filesystem::path& file,
    ActionProgress* ap,
    const OutputProperties& outProps ) const
{
    if (ap) ap->setNSteps(4);
  std::string stem = file.filename().stem().string();

  boost::filesystem::path report_src = (stem+".tex");

  {
      CaseDirectory tmp(false);

      auto report_src_out = tmp/report_src;
      auto dataDir = reportDataPath(report_src_out);

      if (ap) ap->stepUp("Creating temporary directory");
      create_directory ( dataDir );
      for ( auto& i: static_cast<const ResultElement&>(*this) )
      {
          if (auto *re=dynamic_cast<const ResultElement*>(&i))
            re->exportDataToFile ( re->name(), dataDir );
      }

     if (ap) ap->stepUp("Creating LaTeX file");
      writeLatexFile(
          report_src_out,
          ap?ap->forkNewAction(99, "Write LaTeX file").get():nullptr,
          outProps );

      auto outputFileName = report_src_out;
      outputFileName.replace_extension(".pdf");

      if (ap) ap->stepUp("Running PDF compiler");
      bool success=LatexRunner(report_src_out).build();

      insight::assertion(
          boost::filesystem::exists(outputFileName),
          "pdflatex failed to create output file" );

      if (ap) ap->stepUp("Copy data to selected location");
      boost::filesystem::copy_file(
          outputFileName,
          file, copy_option::overwrite_if_exists );

      copyDirectoryRecursively( dataDir, file.parent_path()/dataDir.filename(), false );

      if (!success)
        throw insight::Exception(
              "TeX input file was written but could not execute pdflatex successfully.");

      if (ap) ap->stepUp("Finished");
  }




}




rapidxml::xml_node<> *ResultSet::appendToNode(
    const std::string &name,
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node,
    const OutputProperties& outProps) const
{

    if (p_)
    {
        auto pc = appendNode(doc, node, "parameters");
        p_->appendToNode(std::string(), doc, pc, outProps);
    }

    auto rc=appendNode(doc, node, "results" );
    appendAttribute(doc, rc, "title", title_ );
    appendAttribute(doc, rc, "subtitle", subtitle_ );
    appendAttribute(doc, rc, "date", date_ );
    appendAttribute(doc, rc, "author", author_ );
    appendAttribute(doc, rc, "introduction", introduction_ );

    return ResultElementCollection::appendToNode("", doc, rc, outProps);
}



const rapidxml::xml_node<>* ResultSet::readFromNode(
    const std::string &name,
    const rapidxml::xml_node<> &node )
{
    if (auto *pn = node.first_node("parameters"))
    {
        if (p_)
        {
            p_->readFromNode(std::string(), *pn);
        }
        else
        {
            p_=std::make_unique<ParameterSet>(*pn);
        }
        p_->setParent(this);
    }


    auto *rn = node.first_node("results");
    insight::assertion(
        rn, "mandatory XML node 'results' is missing" );

    title_=getMandatoryAttribute(*rn, "title");
    subtitle_=getMandatoryAttribute(*rn, "subtitle");
    date_=getMandatoryAttribute(*rn, "date");
    author_=getMandatoryAttribute(*rn, "author");
    introduction_=getMandatoryAttribute(*rn, "introduction");

    ResultElementCollection::readFromNode(name, *rn);

    return &node;
}



std::unique_ptr<ParameterSet> ResultSet::convertIntoParameterSet() const
{
    auto ps =ParameterSet::create();

    for ( auto& i: static_cast<const ResultElement&>(*this) )
    {
        if (auto *re=dynamic_cast<const ResultElement*>(&i))
        {
            if ( auto p=re->convertIntoParameter() )
            {
                ps->insert( re->name(), std::move(p) );
            }
        }
    }
    return ps;
}


std::unique_ptr<Parameter> ResultSet::convertIntoParameter() const
{
    return convertIntoParameterSet();
}



int ResultSet::nChildren() const
{
    return ResultElementCollection::nChildren()+(p_?1:0);
}

std::string ResultSet::childElementName(
    int i,
    bool redir ) const
{
    if (p_)
    {
        if (i==0)
            return "InputParameters";
        else
            return ResultElementCollection::childElementName(i-1, redir);
    }
    else
    {
        return ResultElementCollection::childElementName(i, redir);
    }
}

hierarchicalData::Element& ResultSet::childElementRef ( int i )
{
    if (p_)
    {
        if (i==0)
            return *p_;
        else
            return ResultElementCollection::childElementRef(i-1);
    }
    else
    {
        return ResultElementCollection::childElementRef(i);
    }
}


const hierarchicalData::Element& ResultSet::childElement( int i ) const
{
    if (p_)
    {
        if (i==0)
            return *p_;
        else
            return ResultElementCollection::childElement(i-1);
    }
    else
    {
        return ResultElementCollection::childElement(i);
    }
}



std::unique_ptr<hierarchicalData::Element> ResultSet::clone() const
{
     auto nr =std::make_unique<ResultSet> (
            p_ ? p_->cloneAs<ParameterSet>() : std::unique_ptr<ParameterSet>(),
            title_, subtitle_, &author_, &date_ );

    for ( auto& i: static_cast<const ResultElement&>(*this) )
    {
        if (auto *re=dynamic_cast<const ResultElement*>(&i))
            nr->insert ( re->name(), re->cloneAs<ResultElement>() );
    }
    nr->setOrder ( order() );
    nr->introduction() =introduction_;
    return nr;
}


}



