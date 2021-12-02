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
#include "base/exception.h"
#include "openfoam/paraview.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

using namespace std;
using namespace insight;

int main(int argc, char *argv[])
{
    try
    {
        namespace po = boost::program_options;

        // Declare the supported options.
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help,h", "produce help message")
                ("workdir,w", po::value<std::string>(), "execution directory")
                ("statefile,s", po::value<std::string>(), "state file")
                ("batch,b", "batch mode: don't launch GUI, render all views")
                ("parallel,p", "enforce parallel projection in batch rendering")
                ("rescale,c", "automatically rescale all contour plots to data range (within each time step")
                ("onlylatesttime,a", "only select the latest time step  (overrides --to and --from, if they are given)")
                ("from,f", po::value<double>()->default_value(0), "initial time")
                ("to,t", po::value<double>()->default_value(1e10), "final time")
                ;

        po::positional_options_description p;
//            p.add("input-file", -1);

        auto displayHelp = [&]{
            std::ostream &os = std::cout;

            os << "Usage:" << std::endl;
            os << "  " << argv[0] << " [options] " /*<< p.name_for_position(0)*/ << std::endl;
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
            displayHelp();
            exit(-1);
        }

        if (vm.count("help"))
        {
            displayHelp();
            exit(0);
        }

        auto dir = boost::filesystem::current_path();
        if (vm.count("workdir"))
            dir=vm["workdir"].as<std::string>();

        boost::filesystem::path sf;
        if (vm.count("statefile"))
            sf=vm["statefile"].as<std::string>();

        Paraview rp(
           dir, sf,
           vm.count("batch"), vm.count("parallel"),
           vm.count("rescale"), vm.count("onlylatesttime"),
           vm["from"].as<double>(), vm["to"].as<double>()
           );

        rp.wait();

    }
    catch (const std::exception& e)
    {
        printException(e);
        return -1;
    }
}
