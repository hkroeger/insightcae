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
#include "openfoam/ofes.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>


using namespace std;
using namespace insight;
using namespace boost;
namespace bf = boost::filesystem;

int main(int argc, char *argv[])
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    namespace po = boost::program_options;

    typedef std::vector<string> StringList;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "produce help message")

    ("ofe,o", po::value<std::string>(), "use specified OpenFOAM environment instead of detected")

    ("case-dir,l", po::value<std::string>(), "case location")
    ("rename,r", po::value<StringList>(), "rename patch, specify as <current name>:<new name>")
    ;

    po::positional_options_description p;
    p.add("rename", -1);

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

    boost::filesystem::path location=".";
    if (vm.count("case-dir"))
    {
        location = vm["case-dir"].as<std::string>();
    }


    try
    {
        OpenFOAMCase cm( OFEs::getCurrentOrPreferred() );

        OFDictData::dictFile boundaryDict;
        cm.parseBoundaryDict(location, boundaryDict);

        if (vm.count("rename"))
        {
          StringList rename_ops = vm["rename"].as<StringList>();

          for (const auto& s: rename_ops)
          {
            std::vector<std::string> pair;
            boost::split(pair, s, boost::is_any_of(":"));
            if (pair.size()!=2)
                throw insight::Exception
                (
                    "Invalid specification of rename operation in command line!\n"
                    "Each rename operation has to be given as:\n"
                    " <current patch name>:<new patch name>"
                    " (was "+s+")"
                );

            auto i = boundaryDict.find(pair[0]);

            if (i==boundaryDict.end())
              throw insight::Exception("Boundary definition did not contain a patch named "+pair[0]);

            auto value = i->second;
            boundaryDict.erase(i);
            boundaryDict[pair[1]]=value;
          }
        }

        std::ofstream bf( (location/"constant"/"polyMesh"/"boundary").c_str() );
        writeOpenFOAMBoundaryDict(bf, boundaryDict);

    }
    catch (std::exception e)
    {
        insight::printException(e);
        return -1;
    }

    return 0;
}
