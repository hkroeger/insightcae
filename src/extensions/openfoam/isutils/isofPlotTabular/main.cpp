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

#include "boost/filesystem/path.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "insightcaeapplication.h"
#include "plotwidget.h"
#include "isofplottabularwindow.h"
#include "base/exception.h"
#include "qinsighterror.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

using namespace std;

int main(int argc, char *argv[])
{
  qputenv("QT_STYLE_OVERRIDE", 0);

  InsightCAEApplication app(argc, argv, "isofPlotTabular");

  try
  {
    namespace po = boost::program_options;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("input-file,f", po::value< std::vector<boost::filesystem::path> >(),
                        "Input file names. Use \"-\" to read from stdin.")
        ("avg-fraction,a", po::value<double>(), "moving average fraction")
        ("csv-output-file,o", po::value<boost::filesystem::path>(),
                        "output file for final values")
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

    if (!vm.count("input-file"))
    {
        cout<<"input file has to be specified!"<<endl;
        exit(-1);
    }

    auto fns = vm["input-file"].as<std::vector<boost::filesystem::path> >();

    for (const auto& fn: fns)
    {
      if (fn!="-")
      {
        if (!boost::filesystem::exists(fn))
        {
            std::cerr << std::endl
                << "Error: input file does not exist: "<<fn
                <<std::endl<<std::endl;
            exit(-1);
        }
      }
    }

    IsofPlotTabularWindow window(fns);
    window.show();
    if (vm.count("avg-fraction"))
    {
        auto f=vm["avg-fraction"].as<double>();
        QMetaObject::invokeMethod(
            qApp,
            [f,&window]()
            { window.resetAllAvgFractions(f); }
        );
    }
    if (vm.count("csv-output-file"))
    {
        auto fn=vm["csv-output-file"]
                .as<boost::filesystem::path>();
        QObject::connect(
            &window, &IsofPlotTabularWindow::allAverageValuesReady,
            [&window,fn]()
            {
                window.onSaveFinalValues(
                    QString::fromStdString(
                        fn.string() ) );
            }
        );
    }
    return app.exec();
  }
  catch (const std::exception& e)
  {
    displayException(e);;
    return -1;
  }
}
