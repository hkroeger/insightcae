
#include <QApplication>
#include "plotwidget.h"
#include "isofplottabularwindow.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

using namespace std;

int main(int argc, char *argv[])
{
  qRegisterMetaType<arma::mat>();
  qputenv("QT_STYLE_OVERRIDE", 0);

  QApplication app(argc, argv);

  typedef std::vector<std::string> StringList;

  namespace po = boost::program_options;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help", "produce help message")
  ("input-file,f", po::value< StringList >(),"Specifies input file.")
  ;

  po::positional_options_description p;
  p.add("input-file", -1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
      cout << desc << endl;
      exit(-1);
  }

  if (!vm.count("input-file"))
  {
      cout<<"input file has to be specified!"<<endl;
      exit(-1);
  }

  std::string fn = vm["input-file"].as<StringList>()[0];

  if (!boost::filesystem::exists(fn))
  {
      std::cerr << std::endl
          << "Error: input file does not exist: "<<fn
          <<std::endl<<std::endl;
      exit(-1);
  }

  IsofPlotTabularWindow window(fn);
  window.show();
  return app.exec();
}
