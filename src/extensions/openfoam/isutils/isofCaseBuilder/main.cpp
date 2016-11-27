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
#include <QtGui/QApplication>
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




class ISOFApp: public QApplication
{
public:
  ISOFApp ( int &argc, char **argv )
    : QApplication ( argc, argv )
  {}

  ~ISOFApp( )
  {}

  bool notify ( QObject *rec, QEvent *ev )
  {
    try
      {
        return QApplication::notify ( rec, ev );
      }

    catch ( insight::Exception e )
      {
        std::cout << e << std::endl;

        QMessageBox msgBox;
        msgBox.setIcon ( QMessageBox::Critical );
        msgBox.setText ( QString ( e.as_string().c_str() ) );

        msgBox.exec();
      }

    return true;
  }
};




int main ( int argc, char** argv )
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
      ( "batch,b", "case creation from specified input file" )
      ( "skipbcs,s", "skip BC configuration during input file read and batch case creation" )
//       ("workdir,w", po::value<std::string>(), "execution directory")
//       ("savecfg,c", po::value<std::string>(), "save final configuration (including command line overrides) to this file")
//       ("bool,b", po::value<StringList>(), "boolean variable assignment")
//       ("selection,l", po::value<StringList>(), "selection variable assignment")
//       ("string,s", po::value<StringList>(), "string variable assignment")
//       ("path,p", po::value<StringList>(), "path variable assignment")
//       ("double,d", po::value<StringList>(), "double variable assignment")
//       ("int,i", po::value<StringList>(), "int variable assignment")
//       ("merge,m", po::value<StringList>(), "additional input file to merge into analysis parameters before variable assignments")
      ( "input-file,f", po::value< StringList >(),"Specifies input file." )
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

      ISOFApp app ( argc, argv );

      // After creation of application object!
      std::locale::global ( std::locale::classic() );
      QLocale::setDefault ( QLocale::C );

      isofCaseBuilderWindow window;
      if ( vm.count ( "input-file" ) )
        {
          window.loadFile ( vm["input-file"].as<StringList>() [0], vm.count ( "skipbcs" ) );

          if ( vm.count ( "batch" ) )
            {
              window.createCase( vm.count ( "skipbcs" ) );
            }

        }

      if ( !vm.count ( "batch" ) )
      {
        window.show();
        return app.exec();
      }
      else
        return 0;
    }
  catch ( insight::Exception e )
    {
      std::cerr<<e<<std::endl;
    }
}

