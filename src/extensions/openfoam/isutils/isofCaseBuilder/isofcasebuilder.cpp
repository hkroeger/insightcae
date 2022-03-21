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

#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>
#include <QDialog>
#include <QApplication>
#include <QMessageBox>

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#endif

#include <qthread.h>


#include "isofcasebuilderwindow.h"
#include "insightcaeapplication.h"
#include "qinsighterror.h"


using namespace std;
using namespace insight;



insight::ParameterSet& split_and_check
(
    isofCaseBuilderWindow& wnd, 
    std::vector<std::string>& pair,
    const std::string& s
)
{
    boost::split(pair, s, boost::is_any_of(":"));
    if (pair.size()!=3)
        throw insight::Exception
        (
            "Invalid specification of parameter value in command line!\n"
            "Each parameter has to be given as:\n"
            " (#<Case element ID>|<Patch Name>):<parameter set path>:<value>"
        );

    if (pair[0][0]=='#')
    {
        int ceid=boost::lexical_cast<int>(pair[0][1]);
        return wnd.caseElementParameters(ceid);
    } 
    else
    {
        return wnd.BCParameters(pair[0]);
    }
}

int main ( int argc, char** argv )
{
  insight::UnhandledExceptionHandling ueh;
  insight::GSLExceptionHandling gsl_errtreatment;


  bool batch = false;

  InsightCAEApplication app ( argc, argv, "isofCaseBuilder" );

  try
  {

      namespace po = boost::program_options;

      typedef std::vector<std::string> StringList;

      // Declare the supported options.
      po::options_description desc ( "Allowed options" );
      desc.add_options()
      ( "help,h", "produce help message" )
      ( "batch,b", "case creation from specified input file" )
      ( "batch-run,r", "create and run case from specified input file" )
      ( "skipbcs,s", "skip BC configuration during input file read and batch case creation" )
      ( "ofe", po::value<std::string>(),"set the specified OFE for creation and execution" )
      ( "input-file,f", po::value< StringList >(),"Specifies input file. Multiple input files will append to the active configuration." )
      ( "write-only,o", po::value< StringList >(),"restrict output in batch mode to specified files" )
      ("bool", po::value<StringList>(), "boolean variable assignment")
      ("selection,l", po::value<StringList>(), "selection variable assignment")
      ("string", po::value<StringList>(), "string variable assignment")
      ("path,p", po::value<StringList>(), "path variable assignment")
      ("double,d", po::value<StringList>(), "double variable assignment")
      ("vector,v", po::value<StringList>(), "vector variable assignment")
      ("int,i", po::value<StringList>(), "int variable assignment")
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
          return 0;
      }

      batch = vm.count("batch") || vm.count("batch-run");


      // After creation of application object!
      std::locale::global ( std::locale::classic() );
      QLocale::setDefault ( QLocale::C );

      isofCaseBuilderWindow window;

      if ( vm.count ( "input-file" ) )
      {
          for ( const std::string& fn: vm["input-file"].as<StringList>())
          {
              if (!boost::filesystem::exists(fn))
              {
                throw insight::Exception("Input file does not exist: "+fn);
              }
              window.loadFile ( fn, vm.count ( "skipbcs" ) );

          }

          if (vm.count("bool"))
          {
              StringList sets=vm["bool"].as<StringList>();
              for (const string& s: sets)
              {
                  std::vector<std::string> pair;
                  insight::ParameterSet& parameters = split_and_check(window, pair, s);
                  bool v=boost::lexical_cast<bool>(pair[2]);
                  parameters.getBool(pair[1])=v;
              }
          }

          if (vm.count("string"))
          {
              StringList sets=vm["string"].as<StringList>();
              for (const string& s: sets)
              {
                  std::vector<std::string> pair;
                  insight::ParameterSet& parameters = split_and_check(window, pair, s);
                  parameters.getString(pair[1])=pair[2];
              }
          }

          if (vm.count("selection"))
          {
              StringList sets=vm["selection"].as<StringList>();
              for (const string& s: sets)
              {
                  std::vector<std::string> pair;
                  insight::ParameterSet& parameters = split_and_check(window, pair, s);
                  parameters.get<SelectionParameter>(pair[1]).setSelection(pair[2]);
              }
          }

          if (vm.count("path"))
          {
              StringList sets=vm["path"].as<StringList>();
              for (const string& s: sets)
              {
                  std::vector<std::string> pair;
                  insight::ParameterSet& parameters = split_and_check(window, pair, s);
                  parameters.setOriginalFileName(pair[1], pair[2]);
              }
          }

          if (vm.count("double"))
          {
              StringList sets=vm["double"].as<StringList>();
              for (const string& s: sets)
              {
                  std::vector<std::string> pair;
                  insight::ParameterSet& parameters = split_and_check(window, pair, s);
                  double v=toNumber<double>(pair[2]);
                  parameters.getDouble(pair[1])=v;
              }
          }

          if (vm.count("vector"))
          {
              StringList sets=vm["vector"].as<StringList>();
              for (const string& s: sets)
              {
                  std::vector<std::string> pair;
                  insight::ParameterSet& parameters = split_and_check(window, pair, s);
                  arma::mat v;
                  stringToValue(pair[2], v);
                  parameters.getVector(pair[1])=v;
              }
          }

        if (vm.count("int"))
        {
            StringList sets=vm["int"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                insight::ParameterSet& parameters = split_and_check(window, pair, s);
                int v=toNumber<int>(pair[2]);
                parameters.getInt(pair[1])=v;
            }
        }

        if (vm.count("ofe"))
        {
          window.setOFVersion( vm["ofe"].as<std::string>().c_str() );
        }

        if ( batch )
        {
          std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles;

          if ( vm.count ( "write-only" ) )
          {
              restrictToFiles.reset(new std::vector<boost::filesystem::path>);
              StringList paths = vm["write-only"].as<StringList>();
              copy(paths.begin(), paths.end(), std::back_inserter(*restrictToFiles));
          }

          if ( vm.count("batch-run") )
          {
            window.run(isofCaseBuilderWindow::ExecutionStep_Pre, true);
          }
          else
          {
            window.createCase( vm.count ( "skipbcs" ), restrictToFiles );
          }
        }

    }

    if ( !batch )
    {
        window.show();
        return app.exec();
    }
    else
        return 0;
  }
  catch ( const std::exception& e )
  {
    if (batch)
    {
      insight::printException(e);
    }
    else
    {
      displayException(e);
    }
    return -1;
  }
}

