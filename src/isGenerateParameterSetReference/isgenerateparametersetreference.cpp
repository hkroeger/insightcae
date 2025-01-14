
#include <iostream>

#include "base/analysis.h"
#include "base/analysislibrary.h"

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

#include "latexdocumentation.h"
#include "latextable.h"

int main(int argc, char* argv[])
{
  namespace po = boost::program_options;

  typedef std::vector<std::string> StringList;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("libs", po::value< StringList >(),"Additional libraries with analysis modules to load")
    ("analysisname", po::value< std::string >(),"The name of the analysis whichs parameter reference shall be generated")
    ("labelprefix", po::value< std::string >(),"An optional prefix for latex labels")
  ;

  po::positional_options_description p;
  p.add("analysisname", -1);

  auto displayHelp = [&]{
    std::ostream &os = std::cout;

    os << "Usage:" << std::endl;
    os << "  " << boost::filesystem::path(argv[0]).filename().string() << " [options] " << p.name_for_position(0) << std::endl;
    os << std::endl;
    os << desc << std::endl;
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
    std::cerr << std::endl << "Could not parse command line: " << e.what() << std::endl<<std::endl;
    displayHelp();
    exit(-1);
  }

  if (vm.count("help"))
  {
    displayHelp();
    exit(0);
  }


  if (vm.count("libs"))
  {
      StringList libs=vm["libs"].as<StringList>();
      for (const std::string& l: libs)
      {
          if (!boost::filesystem::exists(l))
          {
              std::cerr << std::endl
                  << "Error: library file does not exist: "<<l
                  <<std::endl<<std::endl;
              exit(-1);
          }
          insight::AnalysisLibraryLoader::analysisLibraries().addLibrary(l);
      }
  }

  std::string labelprefix="";
  if (vm.count("labelprefix"))
  {
    labelprefix = vm["labelprefix"].as<std::string>();
  }

  auto analysisName = vm["analysisname"].as<std::string>();
  auto ps = insight::Analysis::defaultParametersFor(analysisName);

  LatexDocumentation doc(*ps, labelprefix);
  doc.print(std::cout, labelprefix);

  return 0;
}
