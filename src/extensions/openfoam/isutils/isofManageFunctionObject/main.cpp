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

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"
#include "openfoam/ofes.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#endif

using namespace boost;
using namespace insight;
using namespace rapidxml;

void evaluateFO(boost::filesystem::path cfgfile, bool skiplatex)
{

  std::string contents;
  readFileIntoString(cfgfile, contents);

    xml_document<> doc;
    doc.parse<0>(&contents[0]);
    
    xml_node<> *rootnode = doc.first_node("root");
    
    OpenFOAMCase cm(OFEs::getCurrentOrPreferred());
    
    ResultSetPtr results(new ResultSet(ParameterSet(), "Evaluation of function objects defined in "+SimpleLatex(cfgfile.string()).toLaTeX(), "Result Report"));
    Ordering o;
  
    // go through all defined case elements. Evaluate all FOs
    for (xml_node<> *e = rootnode->first_node("OpenFOAMCaseElement"); e; e = e->next_sibling("OpenFOAMCaseElement"))
    {
        std::string FOtype = e->first_attribute("type")->value();
	if (outputFilterFunctionObject::factories_->find(FOtype) != outputFilterFunctionObject::factories_->end())
	{
	  ParameterSet ps = outputFilterFunctionObject::defaultParameters(FOtype);
      ps.readFromNode( *e, cfgfile.parent_path() );
	  std::shared_ptr<outputFilterFunctionObject> fo(outputFilterFunctionObject::lookup(FOtype, cm, ps));
	  fo->evaluate
	  (
	    cm, boost::filesystem::current_path(), results, 
	    "Evaluation of function object "+ps.get<StringParameter>("name")()
	  );
	}
    }    
    
    std::string filestem=cfgfile.stem().string();
    boost::filesystem::path resoutpath=cfgfile.parent_path()/(filestem+".isr");
    results->saveToFile( resoutpath );

    boost::filesystem::path outpath=cfgfile.parent_path()/(filestem+".tex");
    results->writeLatexFile( outpath );
   
    if (!skiplatex)
    {
     for (int i=0; i<2; i++)
     {
       if ( ::system( str( format("cd %s && pdflatex \"%s\"") % boost::filesystem::absolute(outpath.parent_path()).string() % outpath.string() ).c_str() ))
       {
 	Warning("TeX input file was written but could not execute pdflatex successfully.");
 	break;
       }
     }
    }

    std::cout
     << "#### ANALYSIS FINISHED SUCCESSFULLY. ####"
     <<std::endl;
}


int main(int argc, char*argv[])
{
  try
  {
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    namespace po = boost::program_options;

    typedef std::vector<std::string> StringList;

    // Declare the supported options.
    po::options_description desc ( "Allowed options" );
    desc.add_options()
    ( "help,h", "produce help message" )
    ( "evaluate,e", po::value<std::string>(), "evaluate function object from specified configuration file" )
    ("skiplatex,x", "skip execution of pdflatex")
    ( "input-file,f", po::value< StringList >(),"Specifies input file. Multiple input files will append to the active configuration." )
    ;

    po::positional_options_description p;
    p.add ( "input-file", -1 );

    auto displayHelp = [&]{
      std::ostream &os = std::cout;

      os << "Usage:" << std::endl;
      os << "  " << boost::filesystem::path(argv[0]).filename().string() << " [options] " << p.name_for_position(0) << std::endl;
      os << std::endl;
      os << desc << endl;
    };

    po::variables_map vm;
    try
    {
      po::store
      (
        po::command_line_parser ( argc, argv ).options ( desc ).positional ( p ).run(),
        vm
      );
      po::notify ( vm );
    }
    catch (const po::error& e)
    {
      std::cerr << std::endl << "Could not parse command line: " << e.what() << std::endl<<std::endl;
      displayHelp();
      exit(-1);
    }

    if ( vm.count ( "help" ) )
      {
        displayHelp();
        exit (0);
      }

    if ( vm.count( "evaluate") )
      {
        boost::filesystem::path cfgfile( vm["evaluate"].as<std::string>() );
        evaluateFO(cfgfile, vm.count("skiplatex"));
      }
  }
  catch ( const std::exception& e )
  {
    insight::printException(e);
    return -1;
  }

  return 0;
}
