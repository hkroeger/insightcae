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

#include <QApplication>
#include <QMainWindow>
#include <QSplashScreen>

#include "base/boost_include.h"
#include "base/exception.h"
#include "base/toolkitversion.h"
#include "base/translations.h"
#include "insightcaeapplication.h"
#include "iqiscadmainwindow.h"
#include "qinsighterror.h"

#ifndef Q_MOC_RUN
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#endif

#include "parser.h"

#include <locale>
#include <QLocale>

#include <qthread.h>

using namespace std;

int main ( int argc, char** argv )
{
  GettextInit iscad_gettextInit(GETTEXT_DOMAIN, GETTEXT_OUTPUT_DIR, GettextInit::Application);

  namespace po = boost::program_options;

  // Declare the supported options.
  po::options_description desc ( _("Allowed options") );
  desc.add_options()
      ( "help,h", _("produce help message") )
      ( "version,r", _("print version and exit") )
      ( "batch,b", _("evaluate model from specified input file without starting GUI") )
      ( "nolog,l", _("put debug output to console instead of log window") )
      ( "nobgparse,g", _("deactivate background parsing") )
      ( "input-file,f", po::value< std::string >(), _("Specify input file.") )
  ;

  po::positional_options_description p;
  p.add ( "input-file", -1 );

  auto displayHelp = [&]{
    std::ostream &os = std::cout;

      os << _("Usage:") << std::endl;
    os << "  " << boost::filesystem::path(argv[0]).filename().string() << " ["<<_("options")<<"] " << p.name_for_position(0) << std::endl;
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
    std::cerr << std::endl << _("Could not parse command line")<<": " << e.what() << std::endl<<std::endl;
    displayHelp();
    exit(-1);
  }

  if ( vm.count ( "help" ) )
    {
      displayHelp();
      exit ( 0 );
    }

  if ( vm.count ( "version" ) )
    {
      cout << std::string(insight::ToolkitVersion::current()) << endl;
      exit ( 0 );
    }

  bool batch = vm.count("batch");

  if ( vm.count ( "input-file" ) && batch )
  {
    try
    {
      boost::filesystem::path filename( vm["input-file"].as<std::string>() );

      insight::cad::ModelPtr model ( new insight::cad::Model );
      bool success;
      if (filename=="-")
      {
        success=insight::cad::parseISCADModelStream ( std::cin, model.get() );
      }
      else if ( boost::filesystem::extension(filename) == ".iscad" )
      {
        success=insight::cad::parseISCADModelFile ( filename, model.get() );
      }
      else
      {
        std::string script = "model: import(\""+filename.string()+"\");\n";
        std::istringstream ms(script);
        success=insight::cad::parseISCADModelStream ( ms, model.get() );
      }
      
      if ( success )
      {
        auto postprocActions=model->postprocActions();
        for ( decltype ( postprocActions ) ::value_type const& v: postprocActions )
        {
            cout << _("Executing")<<" " << v.first << endl;
          v.second->execute();
        }

        return 0;
      }
      else
      {
        std::cerr<<_("Failed to parse ISCAD script!")<<std::endl;
        return -1;
      }
    }
    catch (const std::exception& e)
    {
      insight::printException(e);
      return -1;
    }
  }
  else
  {
    //     XInitThreads();

    InsightCAEApplication app ( argc, argv, "isCAD" );

    try
    {
//      std::locale::global ( std::locale::classic() );
//      QLocale::setDefault ( QLocale::C );

      QPixmap pixmap ( ":/resources/insight_cad_splash.png" );
      QSplashScreen splash ( pixmap, Qt::WindowStaysOnTopHint|Qt::SplashScreen );
      splash.show();
      app.setSplashScreen(&splash);
      splash.showMessage ( QString(_("Please wait"))+"..." );

      IQISCADMainWindow window ( nullptr, vm.count ( "nolog" ) );
      
      bool dobgparsing = (vm.count ( "nobgparse" ) == 0);
      
      if ( vm.count ( "input-file" ) )
      {
        boost::filesystem::path filename ( vm["input-file"].as<std::string>() );
        if ( boost::filesystem::extension(filename) == ".iscad" )
        {
          window.insertModel ( filename, dobgparsing );
        }
        else
        {
          std::string script = "model: import(\""+filename.string()+"\");\n";
          window.insertModelScript ( script, dobgparsing );
        }
      }
      else
      {
        window.insertEmptyModel( dobgparsing );
      }

      window.show();

      window.raise();

      return app.exec();
    }
    catch (const std::exception& e)
    {
        displayException(e);
        return -1;
    }
  }
}
