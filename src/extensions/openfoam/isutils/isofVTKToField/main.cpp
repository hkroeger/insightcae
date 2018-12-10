
#include "base/boost_include.h"
#include "openfoam/openfoamtools.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

using namespace std;

int main(int argc, char* argv[])
{
  insight::UnhandledExceptionHandling ueh;
  insight::GSLExceptionHandling gsl_errtreatment;

  namespace po = boost::program_options;

  typedef std::vector<string> StringList;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", "produce help message")
  ("vtk,v", po::value<std::string>()->required(), "VTK input file name")
  ("out,o", po::value<std::string>(), "output file name")
  ("field,n", po::value<std::string>()->required(), "cell field name")
  ;

  po::positional_options_description p;
  p.add("vtk", 1);
  p.add("field", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
      cout << desc << endl;
      exit(-1);
  }

//  if (vm.count("vtk")!=1)
//  {
//    cout<<"Please specifiy exactly one VTK input file name!"<<endl;
//    return -1;
//  }
//  if (vm.count("field")!=1)
//  {
//    cout<<"Please specifiy exactly one VTK cell field name!"<<endl;
//    return -1;
//  }


  try
  {
    std::ostream* os = &std::cout;
    if (vm.count("out")>0)
    {
      os=new std::ofstream(vm["out"].as<std::string>().c_str());
    }
    insight::VTKFieldToOpenFOAMField(
        vm["vtk"].as<std::string>(),
        vm["field"].as<std::string>(),
        *os
     );
    if (os!=&std::cout) delete os;
  }
  catch (insight::Exception e)
  {
    cerr << "Error occurred:\n" << e << endl;
  }
}

