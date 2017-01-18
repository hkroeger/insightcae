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
#include "openfoam/openfoamtools.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#endif

using namespace boost;
using namespace insight;
using namespace rapidxml;

void reconstructAndClearProcDirs(boost::filesystem::path cd)
{
  
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
      ( "search,s", po::value<std::string>(), "search for cases below the specified path" )
      ;

      po::positional_options_description p;
      p.add ( "input-dir", -1 );

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
        
      if ( vm.count("input-dir") )
      {
	boost::filesystem::path cp( vm["input-dir"].as<std::string>() );
	reconstructAndClearProcDirs(cp);
      }
      
      if ( vm.count("search") )
      {
	boost::filesystem::path ssp( vm["search"].as<std::string>() );
	std::vector<boost::filesystem::path> cases = searchOFCasesBelow(ssp);
	BOOST_FOREACH(const boost::filesystem::path& cp, cases)
	{
	  reconstructAndClearProcDirs(cp);
	}
      }
      
      if ( !vm.count("input-dir") && !vm.count("search") )
      {
	boost::filesystem::path cp = boost::filesystem::current_path();
	reconstructAndClearProcDirs(cp);
      }
      
    }
  catch ( insight::Exception e )
    {
      std::cerr<<e<<std::endl;
    }
  return 0;
}
