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


#include "base/parameters.h"
#include "base/cppextensions.h"
#include "base/translations.h"
#include "base/toolkitversion.h"
#include "base/tools.h"
#include "base/analysis.h"
#include "base/exception.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "boost/format.hpp"

using namespace std;
using namespace insight;
using namespace boost;

int main(int argc, char *argv[])
{
    GettextInit gti(GETTEXT_DOMAIN, GETTEXT_OUTPUT_DIR, GettextInit::Application);

    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    using namespace rapidxml;
    namespace po = boost::program_options;

    typedef std::vector<string> StringList;

    // Declare the supported options.
    po::options_description desc(_("Allowed options"));
    desc.add_options()
        ("help,h", _("produce help message"))
        ("version,r", _("print version and exit"))
        ("print-bool,b", po::value<StringList>(), _("boolean variable assignment"))
        ("print-selection,l", po::value<StringList>(), _("selection variable assignment"))
        ("print-string,s", po::value<StringList>(), _("string variable assignment"))
        ("print-path,p", po::value<StringList>(), _("path variable assignment"))
        ("print-double,d", po::value<StringList>(), _("double variable assignment"))
        ("print-vector,v", po::value<StringList>(), _("vector variable assignment"))
        ("print-int,i", po::value<StringList>(), _("int variable assignment"))
        ("merge,m", po::value<StringList>(), _("additional input file to merge into analysis parameters before variable assignments"))
        ("libs", po::value< StringList >(), _("Additional libraries with analysis modules to load"))
        ("input-file,f", po::value< std::string >(),_("Specifies input file."))
        ;

    po::positional_options_description p;
    p.add("input-file", -1);

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
                po::command_line_parser(argc, argv)
                    .options(desc)
                    .positional(p).run(),
                vm
                );
        po::notify(vm);
    }
    catch (const po::error& e)
    {
        std::cerr << std::endl << _("Could not parse command line")<<": "<< e.what() << std::endl<<std::endl;
        displayHelp();
        exit(-1);
    }

    try
    {
        if (vm.count("help"))
        {
            displayHelp();
            exit(0);
        }

        if (vm.count("version"))
        {
            cout << std::string(insight::ToolkitVersion::current()) << endl;
            exit(0);
        }

        if (vm.count("input-file")<1)
        {
            cerr<<_("an input file has to be specified!")<<endl;
            exit(-1);
        }

        std::string contents, analysisName;

        boost::filesystem::path fn = vm["input-file"].as<std::string>();
        auto inputFileParentPath = boost::filesystem::absolute(fn).parent_path();
        auto filestem = fn.stem().string();

        if (!boost::filesystem::exists(fn))
        {
            std::cerr << std::endl
                      << _("Error: input file does not exist")<<": "<<fn
                      <<std::endl<<std::endl;
            exit(-1);
        }

        readFileIntoString(fn, contents);

        xml_document<> doc;
        doc.parse<0>(&contents[0]);

        xml_node<> *rootnode = doc.first_node("root");

        xml_node<> *analysisnamenode = rootnode->first_node("analysis");
        if (analysisnamenode)
        {
            analysisName = analysisnamenode->first_attribute("name")->value();
        }


        auto parameters =
            insight::Analysis::defaultParameters()(analysisName);

        parameters->readFromNode( std::string(), *rootnode, inputFileParentPath );

        if (vm.count("merge"))
        {
            StringList ists=vm["merge"].as<StringList>();
            for (const string& ist: ists)
            {
                std::vector<std::string> cargs;
                boost::split(cargs, ist, boost::is_any_of(":"));
                if (cargs.size()==1)
                {
                    // 	ParameterSet to_merge;
                    parameters->readFromFile(ist);
                }
                else if (cargs.size()==3)
                {
                    parameters->getParameter(cargs[2]).readFromFile(cargs[0], cargs[1]);
                }
                else
                {
                    throw insight::Exception(_("merge command needs either one or three arguments!\nGot: %s"),
                                             ist.c_str());
                }
            }
        }



        if (vm.count("print-bool"))
        {
            StringList sets=vm["print-bool"].as<StringList>();
            for (const string& s: sets)
            {
                std::cout<<parameters->getBool(s)<<std::endl;
            }
        }

        if (vm.count("print-string"))
        {
            StringList sets=vm["print-string"].as<StringList>();
            for (const string& s: sets)
            {
                std::cout<<parameters->getString(s)<<std::endl;
            }
        }

        if (vm.count("print-selection"))
        {
            StringList sets=vm["print-selection"].as<StringList>();
            for (const string& s: sets)
            {
                std::cout<<parameters->get<SelectionParameter>(s).selection()<<std::endl;
            }
        }

        if (vm.count("print-path"))
        {
            StringList sets=vm["print-path"].as<StringList>();
            for (const string& s: sets)
            {
                std::cout<<parameters->getPath(s)<<std::endl;
            }
        }

        if (vm.count("print-double"))
        {
            StringList sets=vm["print-double"].as<StringList>();
            for (const string& s: sets)
            {
                std::cout<<parameters->getDouble(s)<<std::endl;
            }
        }

        if (vm.count("print-vector"))
        {
            StringList sets=vm["print-vector"].as<StringList>();
            for (const string& s: sets)
            {
                auto v= parameters->getVector(s);
                for (int i=0; i<v.n_elem; ++i)
                {
                    if (i>0) std::cout<<" ";
                    std::cout<<v(i);
                }
                std::cout<<std::endl;
            }
        }

        if (vm.count("print-int"))
        {
            StringList sets=vm["print-int"].as<StringList>();
            for (const string& s: sets)
            {
                std::cout<<parameters->getInt(s)<<std::endl;
            }
        }

        return 0;
    }
    catch (std::exception& ex)
    {
        insight::printException(ex);
        return -1;
    }
}
