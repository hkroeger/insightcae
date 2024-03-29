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

#include "insightcaeapplication.h"
#include "of_clean_case.h"

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

    ("pack,p", "pack case into archive before any cleanup")
    ("pack-file", po::value<std::string>(), "name of archive")

    ("clean-timesteps,t", "clean all time steps (all time steps, if not -0 is given)")
    ("clean-post,s", "clean postprocessing directories")
    ("clean-proc,r", "clean processor directories")
    ("clean-all,c", "clean all time steps, postprocessing data, processor directories and constant+system folder")
    ("keep-first-time,0", "keep first time directory")
    ("clean-inconsistent-parallel-times,i", "remove inconsistent parallel time directories")
    ("dry-run,d", "don't delete anything, just priint the files and directories to be deleted")
    ;

    po::positional_options_description p;
    p.add("case-dir", -1);

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
        insight::OpenFOAMCaseDirs cf(cm, location);

        bool pack=vm.count("pack");
        if (pack)
        {
            bf::path archive_file;

            if (vm.count("pack-file"))
            {
              archive_file = vm["pack-file"].as<std::string>();
            }
            else
            {

              boost::posix_time::ptime t( boost::posix_time::microsec_clock::local_time() );
              boost::posix_time::time_facet *facet = new boost::posix_time::time_facet();
              facet->format( ( bf::current_path().filename().string()+"_%Y-%m-%d-%H.%M" ).c_str() );

              std::ostringstream stream;
              stream.imbue(std::locale(std::locale::classic(), facet));
              stream << t;

              archive_file = location / (stream.str()+".tar.gz");
            }

            cf.packCase(archive_file, insight::OpenFOAMCaseDirs::TimeDirOpt::OnlyFirstAndLast);
        }

        insight::OpenFOAMCaseDirs::TimeDirOpt cto = insight::OpenFOAMCaseDirs::TimeDirOpt::All;
        if (vm.count("keep-first-time"))
        {
          cto=insight::OpenFOAMCaseDirs::TimeDirOpt::ExceptFirst;
        }

        bool dryrun=(vm.count("dry-run")>0);
        bool cleanproc=(vm.count("clean-proc")>0)||(vm.count("clean-all")>0);
        bool cleantimes=(vm.count("clean-timesteps")>0)||(vm.count("clean-all")>0);
        bool cleanpost=(vm.count("clean-post")>0)||(vm.count("clean-all")>0);
        bool cleanall=(vm.count("clean-all")>0);
        bool cleaninsconsistentpartimes=(vm.count("clean-inconsistent-parallel-times")>0);

        if (cleanproc||cleantimes||cleanpost||cleanall||cleaninsconsistentpartimes)
        {
          if (dryrun)
          {
              auto cands=cf.caseFilesAndDirs
                 (
                   cto,
                   cleanproc,
                   cleantimes,
                   cleanpost,
                   cleanall,
                   cleaninsconsistentpartimes
                 );
              for (const auto& c: cands)
              {
                  std::cout<<c<<std::endl;
              }
          }
          else
          {
           cf.cleanCase
              (
                cto,
                cleanproc,
                cleantimes,
                cleanpost,
                cleanall,
                cleaninsconsistentpartimes
              );
          }
        }
        else if (!pack)
        {
          InsightCAEApplication app(argc, argv, "isofCleanCase");
          OFCleanCaseDialog dlg(cm, ".");
          dlg.exec();
        }

    }
    catch (const std::exception& e)
    {
        printException(e);
        exit(-1);
    }

    return 0;
}
