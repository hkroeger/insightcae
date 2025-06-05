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
#include "base/analysislibrary.h"
#include "base/tools.h"
#include "base/parameters.h"

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
    ("unpackexternals,u", "unpack all included external files from ist file")
    ("stripexternals,x", "strip included external files from ist file")
    ("write,w", po::value<std::string>()->required(), "save final configuration (including command line overrides) to this file")
    ("bool,b", po::value<StringList>(), "boolean variable assignment")
    ("selection,l", po::value<StringList>(), "selection variable assignment")
    ("string,s", po::value<StringList>(), "string variable assignment")
    ("path,p", po::value<StringList>(), "path variable assignment")
    ("double,d", po::value<StringList>(), "double variable assignment")
    ("vector,v", po::value<StringList>(), "vector variable assignment")
    ("int,i", po::value<StringList>(), "int variable assignment")
    ("merge,m", po::value<StringList>(), "additional input file to merge into analysis parameters before variable assignments")
    ("libs", po::value< StringList >(),"Additional libraries with analysis modules to load")
    ("input-file,f", po::value<std::string>()->required(),"Specifies input file.")
    ;

    po::positional_options_description p;
    p.add("input-file", 1);
    p.add("write", 1);

    po::variables_map vm;
    try
    {
      po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);
      po::notify(vm);
    }
    catch(std::exception& e)
      {
        std::cerr << "Unhandled Exception reached the top of main: "
                  << e.what() << ", application will now exit" << std::endl;
        return -2;

      }

    if (vm.count("help"))
    {
        cout << desc << endl;
        exit(-1);
    }

    if (vm.count("input-file")!=1)
    {
        cout<<"one single input file has to be specified!"<<endl;
        exit(-1);
    }

    if (vm.count("write")!=1)
    {
        cout<<"one single output file has to be specified (--write) !"<<endl;
        exit(-1);
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
            AnalysisLibraryLoader::analysisLibraries().addLibrary(l);
        }
    }

    try
    {
        std::string fn = vm["input-file"].as<std::string>();

        insight::XMLDocument doc(fn);

        std::string analysisName;

        if (auto *analysisnamenode = doc.rootNode->first_node("analysis"))
        {
            analysisName = analysisnamenode->first_attribute("name")->value();
        }

        auto parameters =
            insight::Analysis::defaultParameters()(analysisName);

        parameters->readFromNode(
            std::string(), *doc.rootNode,
            boost::filesystem::absolute(boost::filesystem::path(fn)).parent_path() );

        if (vm.count("unpackexternals"))
        {
            std::cout<<"Unpacking external files..."<<std::endl;
            parameters->unpack(".");
        }

        if (vm.count("stripexternals"))
        {
            std::cout<<"Removing packed external files..."<<std::endl;
            parameters->clearPackedData();
        }

        if (vm.count("merge"))
        {
            StringList ists=vm["merge"].as<StringList>();
            for (const string& ist: ists)
            {
                std::cout<<"Merging..."<<ist <<std::endl;
                // ParameterSet to_merge;
                parameters->readFromFile(ist);
            }
        }

        if (vm.count("bool"))
        {
            StringList sets=vm["bool"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                bool v=boost::lexical_cast<bool>(pair[1]);
                cout << "Setting boolean '"<<pair[0]<<"' = "<<v<<endl;
                parameters->setBool(pair[0], v);
            }
        }

        if (vm.count("string"))
        {
            StringList sets=vm["string"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                cout << "Setting string '"<<pair[0]<<"' = \""<<pair[1]<<"\""<<endl;
                parameters->setString(pair[0], pair[1]);
            }
        }

        if (vm.count("selection"))
        {
            StringList sets=vm["selection"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                cout << "Setting selection '"<<pair[0]<<"' = \""<<pair[1]<<"\""<<endl;
                parameters->get<SelectionParameter>(pair[0]).setSelection(pair[1]);
            }
        }

        if (vm.count("path"))
        {
            StringList sets=vm["path"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                cout << "Setting path '"<<pair[0]<<"' = \""<<pair[1]<<"\""<<endl;
                //parameters.getPath(pair[0])=pair[1];
                parameters->setOriginalFileName(pair[0], pair[1]);
            }
        }

        if (vm.count("double"))
        {
            StringList sets=vm["double"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                double v=toNumber<double>(pair[1]);
                cout << "Setting double '"<<pair[0]<<"' = "<<v<<endl;
                parameters->setDouble(pair[0], v);
            }
        }

        if (vm.count("vector"))
        {
            StringList sets=vm["vector"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                arma::mat v;
                stringToValue(pair[1], v);
                cout << "Setting vector '"<<pair[0]<<"' = "<<v<<endl;
                parameters->setVector(pair[0], v);
            }
        }

        if (vm.count("int"))
        {
            StringList sets=vm["int"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                int v=toNumber<int>(pair[1]);
                cout << "Setting int '"<<pair[0]<<"' = "<<v<<endl;
                parameters->setInt(pair[0], v);
            }
        }

        boost::filesystem::path outfile = vm["write"].as<std::string>();
        std::cout << "Saving modified input parameters to file "<<outfile<<std::endl;
        parameters->saveToFile( outfile, analysisName );

    }
    catch (insight::Exception e)
    {
        cout<<"Exception occured: "<<e<<endl;
        exit(-1);
    }

    return 0;
}
