/*
 * Manual test tool for insight::ConvergenceByVariance.
 *
 * Reads the time histories of a set of cases from a directory:
 * each subdirectory is one case. All files in a case directory are read
 * (first column: time, remaining columns: data). Files in numeric
 * subdirectories (time directories, e.g. from restarts) are combined
 * by name like OpenFOAM postProcessing output.
 * The convergence criterion is evaluated for each column of each file.
 * Deselected columns (stored in <case-dir>/convergencebyvariance.cfg)
 * are ignored in the convergence status of the case.
 *
 *  batch mode (-b): print convergence status per case and series
 *  GUI mode:        one tab per case (green: converged, yellow: converged
 *                   earlier but diverged again, red: never converged),
 *                   global evaluation parameters, interactive selection
 *                   of the evaluated time window
 */

#include "casereader.h"
#include "evaluate.h"
#include "convergencewindow.h"

#include "insightcaeapplication.h"
#include "qinsighterror.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <limits>
#include <cmath>

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "produce help message")
      ("batch,b", "batch mode: print convergence status per case and series and exit")
      ("fraction,a", po::value<double>()->default_value(0.2),
                     "trailing window size as fraction of the number of evaluated samples")
      ("threshold,c", po::value<double>()->default_value(1e-2),
                     "a series is converged, if its final CoV is below this value")
      ("min-window,w", po::value<double>()->default_value(50),
                     "... and if its trailing window is at least this large (see --window-unit)")
      ("window-unit,u", po::value<std::string>()->default_value("samples"),
                     "unit of --min-window: 'samples' (number of samples in trailing window, >=2) "
                     "or 'time' (time span covered by trailing window, >0)")
      ("start-time,s", po::value<double>(), "start of evaluated time window (default: first time of each series)")
      ("end-time,e", po::value<double>(), "end of evaluated time window (default: last time of each series)")
      ("case-dir,d", po::value<boost::filesystem::path>(),
                     "directory containing one subdirectory per case")
  ;

  po::positional_options_description p;
  p.add("case-dir", 1);

  auto displayHelp = [&]{
    std::cout << "Usage:" << std::endl
              << "  " << boost::filesystem::path(argv[0]).filename().string()
              << " [options] case-dir" << std::endl << std::endl
              << desc << std::endl;
  };

  po::variables_map vm;
  try
  {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e)
  {
    std::cerr << std::endl << "Could not parse command line: " << e.what() << std::endl << std::endl;
    displayHelp();
    return -1;
  }

  if (vm.count("help"))
  {
    displayHelp();
    return 0;
  }

  if (!vm.count("case-dir"))
  {
    std::cerr << "case directory has to be specified!" << std::endl;
    displayHelp();
    return -1;
  }

  bool batch = vm.count("batch");

  // NaN: use time range of each series
  const double undefined = std::numeric_limits<double>::quiet_NaN();
  double t0 = vm.count("start-time") ? vm["start-time"].as<double>() : undefined;
  double t1 = vm.count("end-time") ? vm["end-time"].as<double>() : undefined;
  double fraction = vm["fraction"].as<double>();
  Criterion crit;
  crit.threshold = vm["threshold"].as<double>();
  crit.minWindow = vm["min-window"].as<double>();
  std::string windowUnit = vm["window-unit"].as<std::string>();

  std::vector<CaseData> cases;
  boost::filesystem::path dir;
  try
  {
    if (!(fraction>0. && fraction<=1.))
      throw std::runtime_error("fraction has to be in range (0, 1]");
    if (!(crit.threshold>0.))
      throw std::runtime_error("threshold has to be positive");
    if (windowUnit=="samples")
    {
      crit.unit = WindowUnit::Samples;
      // the first sample (window of 1) has no variance (CoV=0 is a placeholder)
      if (!(crit.minWindow>=2.) || crit.minWindow!=std::round(crit.minWindow))
        throw std::runtime_error("min-window has to be an integer >= 2 for window unit 'samples'");
    }
    else if (windowUnit=="time")
    {
      crit.unit = WindowUnit::Time;
      if (!(crit.minWindow>0.))
        throw std::runtime_error("min-window has to be positive for window unit 'time'");
    }
    else
      throw std::runtime_error("window-unit has to be 'samples' or 'time'");

    dir = vm["case-dir"].as<boost::filesystem::path>();
    std::cerr << "Reading cases from " << dir.string() << " ..." << std::endl;
    cases = readCases(dir);

    for (const auto& cd: cases)
    {
      size_t nSel=0;
      for (const auto& s: cd.series) if (s.selected) ++nSel;
      std::cerr << "  " << cd.name << ": " << cd.series.size() << " series";
      if (nSel<cd.series.size())
        std::cerr << " (" << cd.series.size()-nSel << " deselected)";
      std::cerr << std::endl;
      for (const auto& w: cd.warnings)
        std::cerr << "    warning: skipped " << w << std::endl;
    }

    if (batch)
    {
      for (const auto& cd: cases)
      {
        auto res = evaluate(cd, fraction, t0, t1);

        size_t nConv=0, nSel=0;
        for (size_t i=0; i<res.size(); ++i)
          if (cd.series[i].selected)
          {
            ++nSel;
            if (converged(res[i], crit)) ++nConv;
          }

        auto st = caseStatus(cd, res, crit);
        double tc = firstConvergedTime(cd, res, crit);
        std::cout << boost::str(boost::format("%s: %s (%d/%d selected series below %g, min. window %s)")
                    % cd.name
                    % ( st==CaseStatus::Converged ? "CONVERGED" :
                        st==CaseStatus::ConvergedEarlier ? "CONVERGED EARLIER, DIVERGED AGAIN" :
                        st==CaseStatus::NotConverged ? "NOT CONVERGED" : "NO SERIES SELECTED" )
                    % nConv % nSel % crit.threshold % minWindowText(crit) );
        if (std::isnan(tc))
          std::cout << ", all selected series never converged simultaneously";
        else
          std::cout << ", all selected series first converged at t=" << tc;
        std::cout << std::endl;

        for (size_t i=0; i<res.size(); ++i)
        {
          const auto& r=res[i];
          std::string status =
              !cd.series[i].selected ? "(deselected)" :
              converged(r, crit) ? "ok" :
              windowTooSmall(r, crit) ? "NOT CONVERGED (window too small)" :
              seriesStatus(r, crit)==SeriesStatus::ConvergedEarlier ? "NOT CONVERGED (converged earlier)" :
                                                                       "NOT CONVERGED";
          if (r.nSamples>0)
            std::cout << boost::str(boost::format(
                  "  %-45s CoV = %-12g (n=%d, t=[%g, %g])  %s")
                  % cd.series[i].label() % r.finalCoV % r.nSamples % r.t0 % r.t1
                  % status )
                      << std::endl;
          else
            std::cout << boost::str(boost::format(
                  "  %-45s no samples in time window t=[%g, %g]  %s")
                  % cd.series[i].label() % r.t0 % r.t1 % status )
                      << std::endl;
        }
      }
      return 0;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
  }

  InsightCAEApplication app(argc, argv, "test_convergencebyvariance");
  try
  {
    ConvergenceWindow window( dir, cases, fraction, crit, t0, t1 );
    window.show();
    return app.exec();
  }
  catch (...)
  {
    displayCurrentException();
    return -1;
  }
}
