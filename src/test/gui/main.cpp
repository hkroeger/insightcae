#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>

#include "insightcaeapplication.h"
#include "base/boost_include.h"
#include "base/toolkitversion.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "mainwindow.h"
#include "qinsighterror.h"

#include "base/exception.h"
#include "base/linearalgebra.h"

#include <qthread.h>

using namespace boost;
using namespace std;

int main(int argc, char *argv[])
{
  insight::UnhandledExceptionHandling ueh;
  insight::GSLExceptionHandling gsl_errtreatment;

  namespace po = boost::program_options;

  typedef std::vector<std::string> StringList;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help", "produce help message")
  ("version,r", "print version and exit")
  ("libs", po::value< StringList >(), "Additional libraries with analysis modules to load")
  ("input-file,f", po::value< std::string >(), "Specifies input file.")
  ;

  po::positional_options_description p;
  p.add("input-file", -1);

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

  InsightCAEApplication app(argc, argv, "InsightCAE GUI Test");
  std::locale::global(std::locale::classic());
  QLocale::setDefault(QLocale::C);

  MainWindow w;
  w.show();
  return app.exec();
}
