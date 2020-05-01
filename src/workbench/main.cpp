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

#include "insightcaeapplication.h"
#include "base/boost_include.h"
#include "base/toolkitversion.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "workbench.h"
#include "qinsighterror.h"

#include "base/analysis.h"
#include "base/exception.h"
#include "base/linearalgebra.h"

#include <qthread.h>
 
 
using namespace boost;
using namespace std;





int main(int argc, char** argv)
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    namespace po = boost::program_options;

    typedef std::vector<std::string> StringList;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce help message")
    ("version,r", "print version and exit")
    ("libs", po::value< StringList >(), "Additional libraries with analysis modules to load")
    ("input-file,f", po::value< std::string >(), "Specifies input file.")
    ;
    
    po::positional_options_description p;
    p.add("input-file", -1);
    
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
      po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);
      po::notify(vm);
    }
    catch (const po::error& e)
    {
      std::cerr << std::endl << "Could not parse command line: " << e.what() << std::endl<<std::endl;
      displayHelp();
      exit(-1);
    }

    if (vm.count("help"))
    {
        displayHelp();
        exit(0);
    }

    if (vm.count("version"))
    {
        cout << std::string(insight::ToolkitVersion::current) << endl;
        exit(0);
    }

    if (vm.count("libs"))
    {
        StringList libs=vm["libs"].as<StringList>();
        for (const string& l: libs)
        {
            if (!boost::filesystem::exists(l))
            {
                std::cerr << std::endl 
                    << "Error: library file does not exist: "<<l
                    <<std::endl<<std::endl;
                exit(-1);
            }
           insight::loader.addLibrary(l);
        }
    }

    InsightCAEApplication app(argc, argv, "InsightCAE Workbench");

    // After creation of application object!
    std::locale::global(std::locale::classic());
    QLocale::setDefault(QLocale::C);

    QPixmap pixmap(":/resources/insight_workbench_splash.png");
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint|Qt::SplashScreen);
    splash.show();
    app.setSplashScreen(&splash);
    splash.showMessage( QString::fromStdString(insight::ToolkitVersion::current) + ", Wait...");

    workbench window;

    try
    {
      if (vm.count("input-file"))
      {
          boost::filesystem::path fn( vm["input-file"].as<std::string>() );
          if (!boost::filesystem::exists(fn))
          {
              throw insight::Exception("Input file does not exist: "+fn.string());
          }
          window.openAnalysis(boost::filesystem::absolute(fn).c_str());
      }
      window.show();

      app.processEvents();//This is used to accept a click on the screen so that user can cancel the screen

      window.raise();

      return app.exec();
    }
    catch ( const std::exception& e)
    {
      displayException(e);
    }

}
