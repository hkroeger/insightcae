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

#include <boost/concept_check.hpp>

#include "base/linearalgebra.h"
#include "base/analysis.h"
#include "base/resultset.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "boost/format.hpp"

using namespace std;
using namespace insight;
using namespace boost;

int main(int argc, char *argv[])
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    using namespace rapidxml;
    namespace po = boost::program_options;

    typedef std::vector<string> StringList;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce help message")
    ("libs", po::value< StringList >(),"Additional libraries with analysis modules to load")
    ("list", po::value< std::string>(),"List contents of result file")
//     ("combineplots", po::value< StringList >(),"Additional libraries with analysis modules to load")
    ;

    po::positional_options_description p;
//     p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << endl;
        exit(-1);
    }

    try
    {

    if (vm.count("libs"))
    {
        StringList libs=vm["libs"].as<StringList>();
        BOOST_FOREACH(const string& l, libs)
        {
            if (!boost::filesystem::exists(l))
            {
                std::cerr << std::endl 
                    << "Error: library file does not exist: "<<l
                    <<std::endl<<std::endl;
                exit(-1);
            }
            loader.addLibrary(l);
        }
    }
    
    if (vm.count("list"))
    {
        boost::filesystem::path f(vm["list"].as<std::string>());
        ResultElementCollection r;
        r.readFromFile(f);
        BOOST_FOREACH(ResultElementCollection::value_type& rel, r)
        {
            std::cout<<rel.first<<std::endl;
        }
    }
    }
    catch (insight::Exception e)
    {
        std::cerr<<e<<std::endl;
        exit(-1);
    }
}
