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
#include "openfoam/analysiscaseelements.h"
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
    std::ifstream in(cfgfile.c_str());
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    xml_document<> doc;
    doc.parse<0>(&contents[0]);
    
    xml_node<> *rootnode = doc.first_node("root");
    
    std::string cofe=OFEs::detectCurrentOFE();
    if ( cofe == std::string() )
      throw insight::Exception("OpenFOAM environment is not set!");
    OpenFOAMCase cm(OFEs::get(cofe));
    
    ResultSetPtr results(new ResultSet(ParameterSet(), cfgfile.string(), "Result Report"));
    Ordering o;
  
    // go through all defined case elements. Evaluate all FOs
    for (xml_node<> *e = rootnode->first_node("OpenFOAMCaseElement"); e; e = e->next_sibling("OpenFOAMCaseElement"))
    {
        std::string FOtype = e->first_attribute("type")->value();
	if (outputFilterFunctionObject::factories_->find(FOtype) != outputFilterFunctionObject::factories_->end())
	{
	  ParameterSet ps = outputFilterFunctionObject::defaultParameters(FOtype);
	  ps.readFromNode(doc, *e, cfgfile.parent_path());
	  boost::shared_ptr<outputFilterFunctionObject> fo(outputFilterFunctionObject::lookup(FOtype, cm, ps));
	  fo->evaluate
	  (
	    cm, boost::filesystem::current_path(), results, 
	    "Evaluation of FO"
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
       if ( ::system( str( format("cd %s && pdflatex \"%s\"") % outpath.parent_path().string() % outpath.string() ).c_str() ))
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
      //       ( "batch,b", "case creation from specified input file" )
//       ( "skipbcs,s", "skip BC configuration during input file read and batch case creation" )
//       ("workdir,w", po::value<std::string>(), "execution directory")
//       ("savecfg,c", po::value<std::string>(), "save final configuration (including command line overrides) to this file")
//       ("bool,b", po::value<StringList>(), "boolean variable assignment")
//       ("selection,l", po::value<StringList>(), "selection variable assignment")
//       ("string,s", po::value<StringList>(), "string variable assignment")
//       ("path,p", po::value<StringList>(), "path variable assignment")
//       ("double,d", po::value<StringList>(), "double variable assignment")
//       ("int,i", po::value<StringList>(), "int variable assignment")
//       ("merge,m", po::value<StringList>(), "additional input file to merge into analysis parameters before variable assignments")
      ( "input-file,f", po::value< StringList >(),"Specifies input file. Multiple input files will append to the active configuration." )
      ;

      po::positional_options_description p;
      p.add ( "input-file", -1 );

      po::variables_map vm;
      po::store
      (
        po::command_line_parser ( argc, argv ).options ( desc ).positional ( p ).run(),
        vm
      );
      po::notify ( vm );

      if ( vm.count ( "help" ) )
        {
          std::cout << desc << std::endl;
          exit ( -1 );
        }
        
      if ( vm.count( "evaluate") )
        {
	  boost::filesystem::path cfgfile( vm["evaluate"].as<std::string>() );
	  evaluateFO(cfgfile, vm.count("skiplatex"));
	}
    }
  catch ( insight::Exception e )
    {
      std::cerr<<e<<std::endl;
    }
  return 0;
}