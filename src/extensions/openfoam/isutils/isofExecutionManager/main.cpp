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

#include "base/boost_include.h"
#include "base/linearalgebra.h"
#include "base/analysis.h"
#include "openfoam/openfoamtools.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <QApplication>
#include "mainwindow.h"

using namespace std;
using namespace insight;
using namespace boost;
namespace bf = boost::filesystem;


int main(int argc, char *argv[])
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    namespace po = boost::program_options;

//    typedef std::vector<string> StringList;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "produce help message")

    ("ofe,o", po::value<std::string>(), "use specified OpenFOAM environment instead of detected")
    ("case-dir,l", po::value<std::string>(), "case location")
    ;

    po::positional_options_description p;
    p.add("case-dir", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << endl;
        exit(-1);
    }

    boost::filesystem::path location=".";
    if (vm.count("case-dir"))
    {
        location = vm["case-dir"].as<std::string>();
    }


    try
    {
          QApplication app(argc, argv);
          MainWindow w;
          w.show();
          return app.exec();
    }
    catch (insight::Exception e)
    {
        cout<<"Error: "<<e<<endl;
        exit(-1);
    }
    catch (std::exception e)
    {
        cout<<"Error: "<<e.what()<<endl;
        exit(-1);
    }
}
