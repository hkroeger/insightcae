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

#include "base/tools.h"
#include "base/linearalgebra.h"
#include "base/analysis.h"
#include "base/analysislibrary.h"
#include "base/analysisthread.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/streamtoprogressdisplayer.h"
#include "base/toolkitversion.h"
#include "base/parameters.h"
#include "base/cppextensions.h"
#include "base/translations.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "boost/format.hpp"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "boost/asio/io_service.hpp"

#ifdef HAVE_WT
#include "restapi.h"
#include "detectionhandler.h"
#endif


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
      ("skiplatex,x", _("skip execution of pdflatex"))
      ("version,r", _("print version and exit"))
      ("workdir,w", po::value<std::string>(), _("execution directory"))
      ("savecfg,c", po::value<std::string>(), _("save final configuration (including command line overrides) to this file"))
      ("bool,b", po::value<StringList>(), _("boolean variable assignment"))
      ("selection,l", po::value<StringList>(), _("selection variable assignment"))
      ("string,s", po::value<StringList>(), _("string variable assignment"))
      ("path,p", po::value<StringList>(), _("path variable assignment"))
      ("double,d", po::value<StringList>(), _("double variable assignment"))
      ("vector,v", po::value<StringList>(), _("vector variable assignment"))
      ("int,i", po::value<StringList>(), _("int variable assignment"))
      ("set-array-size", po::value<StringList>(), _("set size of array"))
      ("merge,m", po::value<StringList>(), _("additional input file to merge into analysis parameters before variable assignments"))
      ("libs", po::value< StringList >(), _("Additional libraries with analysis modules to load"))
      ("input-file,f", po::value< std::string >(),_("Specifies input file."))
      ("output-file,o", po::value< std::string >(),_("Specifies output file. May be a PDF report or an ISR InsightCAE XML result set."))
#ifdef HAVE_WT
      ("server", _("Start with REST API server. Keeps the application running after the analysis has finished. Once the result set is fetched via the REST API, the application exits."))
      ("listen", po::value<std::string>()->default_value("127.0.0.1"), _("Server address"))
      ("port", po::value<int>()->default_value(8090), _("Server port"))
      ("broadcastport", po::value<int>()->default_value(8090), _("Broadcast listen port"))
#endif
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

    boost::filesystem::path workdir = boost::filesystem::current_path();
    std::string filestem = "analysis";

    if (vm.count("workdir"))
    {
        workdir=boost::filesystem::absolute(vm["workdir"].as<std::string>());
        boost::filesystem::current_path(workdir); // make it the current directory
    }

    boost::filesystem::path inputFileParentPath = workdir;

    std::string analysisName = "";

    auto summarizeWarnings = [&]()
    {
      if (WarningDispatcher::getCurrent().nWarnings()>0)
      {
        std::cerr
            << "\n"
            << boost::str(boost::format(
                           _("There have been %d warnings."))
                            % WarningDispatcher::getCurrent().nWarnings() )<<"\n"
            << _("Please review")<<":\n";

        int i=0;
        for (const auto& w: WarningDispatcher::getCurrent().warnings())
        {
            std::cerr<<"\n** "<<_("Warning")<<" "<<(++i)<<":\n";
            displayFramed(_("Warning"), w.what(), '-', std::cout);
        }
      }
    };


    try
    {

#ifdef HAVE_WT
      std::unique_ptr<AnalyzeRESTServer> server;
      std::unique_ptr<DetectionHandler> detectionHandler;

      if (vm.count("server"))
      {

        server.reset(new AnalyzeRESTServer(
                       argv[0],
                       vm["listen"].as<std::string>(),
                       vm["port"].as<int>()
            ));

        if (!server->start())
        {
            std::cerr << _("Could not start web server!") << std::endl;
        }

//// disable for now. not working inside WSL
//#ifndef WIN32
//        detectionHandler = DetectionHandler::start(
//                        vm["broadcastport"].as<int>(),
//                        vm["listen"].as<std::string>(),
//                        vm["port"].as<int>(),
//                        analysisName
//                      );
//#endif
      }
#endif



        if (vm.count("libs"))
        {
            StringList libs=vm["libs"].as<StringList>();
            for (const string& l: libs)
            {
                if (!boost::filesystem::exists(l))
                {
                    std::cerr << std::endl
                              << _("Error: library file does not exist: ")<<l
                        <<std::endl<<std::endl;
                    exit(-1);
                }
                AnalysisLibraryLoader::analysisLibraries().addLibrary(l);
            }
        }
        
        std::string contents;

        if (!vm.count("input-file"))
        {
#ifdef HAVE_WT
          if (server)
          {
            cout<<_("Running in server mode without explicitly specified input file: "
                          "waiting for input transmission")<<endl;
            if (!server->waitForInputFile(contents))
            {
                throw insight::Exception(_("Received interruption!"));
            }
          }
          else
#endif
          {
            cerr<<_("input file has to be specified!")<<endl;
            exit(-1);
          }
        }
        else
        {
          if (vm.count("input-file")>1)
          {
            cerr<<_("only one single input file has to be specified!")<<endl;
            exit(-1);
          }

          boost::filesystem::path fn = vm["input-file"].as<std::string>();
          inputFileParentPath = boost::filesystem::absolute(fn).parent_path();
          filestem = fn.stem().string();

          if (!boost::filesystem::exists(fn))
          {
              std::cerr << std::endl
                      << _("Error: input file does not exist")<<": "<<fn
                  <<std::endl<<std::endl;
              exit(-1);
          }

          readFileIntoString(fn, contents);
        }

        xml_document<> doc;
        doc.parse<0>(&contents[0]);

        xml_node<> *rootnode = doc.first_node("root");

        xml_node<> *analysisnamenode = rootnode->first_node("analysis");
        if (analysisnamenode)
        {
            analysisName = analysisnamenode->first_attribute("name")->value();
        }

        cout<< _("Executing analysis in directory")<<" "<<workdir<<endl;

        ParameterSet parameters = insight::Analysis::defaultParameters(analysisName);
        
        parameters.readFromNode(doc, *rootnode, inputFileParentPath );

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
                  parameters.readFromFile(ist);
                }
                else if (cargs.size()==3)
                {
                    parameters.get<SubParameterSet>(cargs[2]).subsetRef().readFromFile(cargs[0], cargs[1]);
                }
                else
                {
                    throw insight::Exception(_("merge command needs either one or three arguments!\nGot: %s"), ist.c_str());
                }
            }
        }

        if (vm.count("set-array-size"))
        {
            StringList resizes=vm["set-array-size"].as<StringList>();
            for (const string& s: resizes)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                int ns=boost::lexical_cast<int>(pair[1]);
                cout << boost::str(boost::format(
                                       _("Resizing array '%s' to %d")
                                       ) % pair[0] % ns )<<endl;
                auto& ap = parameters.get<ArrayParameter>(pair[0]);
                for (size_t i=ap.size(); i<ns; ++i)
                {
                    ap.appendEmpty();
                }
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
                cout << boost::str(boost::format(
                                       _("Setting boolean '%s' = %d")
                                       )% pair[0] % v) <<endl;
                parameters.getBool(pair[0])=v;
            }
        }

        if (vm.count("string"))
        {
            StringList sets=vm["string"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                cout << boost::str(boost::format(
                                       _("Setting string '%s' = \"%s\"")
                                       ) % pair[0] % pair[1]) <<endl;
                parameters.getString(pair[0])=pair[1];
            }
        }

        if (vm.count("selection"))
        {
            StringList sets=vm["selection"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                cout << boost::str(boost::format(
                                       _("Setting selection '%s' = \"%s\"")
                                       ) % pair[0] % pair[1]) <<endl;
                parameters.get<SelectionParameter>(pair[0]).setSelection(pair[1]);
            }
        }

        if (vm.count("path"))
        {
            StringList sets=vm["path"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                cout << boost::str(boost::format(
                                       _("Setting path '%s' = \"%s\"")
                                       ) % pair[0] % pair[1]) <<endl;
                //parameters.getPath(pair[0])=pair[1];
                parameters.setOriginalFileName(pair[0], pair[1]);
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
                cout << boost::str(boost::format(
                                       _("Setting double '%s' = %g")
                                       ) % pair[0] % v) << endl;
                parameters.getDouble(pair[0])=v;
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
                cout << boost::str(boost::format(
                                        _("Setting vector '%s' = [%g %g %g]")
                                       ) % pair[0] % v(0) % v(1) % v(2)) <<endl;
                parameters.getVector(pair[0])=v;
            }
        }

        if (vm.count("int"))
        {
            StringList sets=vm["int"].as<StringList>();
            for (const string& s: sets)
            {
                std::vector<std::string> pair;
                boost::split(pair, s, boost::is_any_of(":"));
                int v=boost::lexical_cast<int>(pair[1]);
                cout << boost::str(boost::format(
                                       _("Setting int '%s'= %d")
                                       ) % pair[0] % v) <<endl;
                parameters.getInt(pair[0])=v;
            }
        }

        if (vm.count("savecfg"))
        {
            parameters.saveToFile( workdir/ vm["savecfg"].as<std::string>(), analysisName );
        }

        std::cout<<std::string(80, '=')+'\n';
        std::cout<<_("Applied Parameters for this run")<<std::endl;
        std::cout<<parameters;
        std::cout<<std::string(80, '=')+"\n\n";
        std::cout<<std::flush;

        ResultSetPtr results;
        AnalysisPtr analysis;
        try
        {
          TextProgressDisplayer tpd;
          ProgressDisplayer* pd = &tpd;
          std::unique_ptr<StreamToProgressDisplayer> redirCout, redirCerr;
#ifdef HAVE_WT
          if (server)
          {
            pd = server.get();
            redirCout=std::make_unique<StreamToProgressDisplayer>(std::cout, *pd);
//            redirCerr=std::make_unique<StreamToProgressDisplayer>(std::cerr, *pd);
          }
#endif

          analysis.reset( insight::Analysis::lookup(analysisName, parameters, workdir, *pd) );
        
#ifdef HAVE_WT
          if (server)
          {
            server->setAnalysis( analysis.get() );
          }
#endif

          // run analysis

          AnalysisThread solver_thread(analysis, pd);

#ifdef HAVE_WT
          if (server)
          {
            server->setSolverThread(&solver_thread);
          }
#endif

          results = solver_thread.join();
        }
        catch (insight::Exception& ex)
        {
          if (server)
          {
            server->setException(ex);
            server->waitForShutdown();
          }
          else
          {
            // rethrow
            throw ex;
          }
        }

#ifdef HAVE_WT
        if (server)
        {
          server->setSolverThread(nullptr);
        }
#endif

        if (results)
        {
#ifdef HAVE_WT
          if (server)
          {
            server->setResults(results);
            server->waitForShutdown();
          }
          else
#endif
          {
            if (!vm.count("output-file"))
            {
                boost::filesystem::path resoutpath=analysis->executionPath()/ (filestem+".isr");
                results->saveToFile( resoutpath );

                boost::filesystem::path outpath=analysis->executionPath()/ (filestem+".tex");
                results->writeLatexFile( outpath );

                if (!vm.count("skiplatex"))
                {
                    for (int i=0; i<2; i++)
                    {
                        if ( ::system( str( format("cd %s && pdflatex -interaction=batchmode \"%s\"") % workdir.string() % outpath.string() ).c_str() ))
                        {
                            Warning(_("TeX input file was written but could not execute pdflatex successfully."));
                            break;
                        }
                    }
                }
            }
            else
            {
                results->saveAs( vm["output-file"].as<std::string>() );
            }
          }

          std::cout
              << "#### "<<_("ANALYSIS FINISHED SUCCESSFULLY.")<<" ####"
              <<std::endl;
        }
        else
        {
          std::cerr
              << "#### "<<_("ANALYSIS STOPPED WITHOUT RESULTS.")<<" ####"
              <<std::endl;
        }

#ifdef HAVE_WT
        if (server)
        {
          server->stop();
        }
#endif
    }
    catch (const std::exception& e)
    {
      summarizeWarnings();

      std::cerr<<"*** "<<_("The analysis was stopped due to this error:")<<"\n\n";
      printException(e);
      return -1;
    }

    summarizeWarnings();

    return 0;
}
