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

#include "convergenceanalysis.h"

#include "base/boost_include.h"

using namespace boost;
using namespace std;


namespace insight
{
    
addToAnalysisFactoryTable(ConvergenceAnalysis);

ConvergenceAnalysis::ConvergenceAnalysis(const ParameterSet& ps, const boost::filesystem::path& exepath)
: Analysis("Convergence Analysis", "", ps, exepath)
{
}


ResultSetPtr ConvergenceAnalysis::operator()(ProgressDisplayer* displayer)
{
  setupExecutionEnvironment();
  
  Parameters p ( parameters_ );

  if ( p.solutions.size() !=3 )
    {
      throw insight::Exception
      (
        str ( format ( "Error: unsupported number of results for convergence analysis. Has to be 3 but is %d." ) % p.solutions.size() )
      );
    }

  ResultSetPtr results ( new ResultSet ( parameters(), name_, "Result Report" ) );
//   boost::shared_ptr<ResultSection> section
//   (
//     new ResultSection
//     (
//       title,
//
//       str(format(
// 	"This section describes the results of a convergence study of the solution quantity %s according to"
// 	" the procedure described in ITTC 7.5-03-01-01 Rev. 02.\n"
// 	"The convergence study is based on solutions on three gradually refined meshes $i=1,2,3$ with $i=1$ being the index of the finest mesh."
//       ) % name)
//     )
//   );
  Ordering so;

  double
  r1=p.solutions[1].deltax / p.solutions[2].deltax,
  r2=p.solutions[0].deltax / p.solutions[1].deltax;

//   if (inv)
//   {
//     r1=1./r1;
//     r2=1./r2;
//   }

  double r = ( r1 + r2 ) / 2.;

  results->insert
  (
    p.name+"_r",
    new ScalarResult (
      r,
      "Averaged refinement ratio",

      str ( format ( "A uniform refinement ratio between parameter steps is desired. "
                     "Since parameters can be integers, this can not always be strictly fulfilled. "
                     "Thus, the average between the ratio of the finest to medium resolution (here $r_1=%g$) and "
                     "the ratio of medium to the coarsest (here $r_2=%g$) is used." ) % r1 % r2 ),

      ""
    )
  ).setOrder ( so.next() );

  double
  S1=p.solutions[2].S, // finest
  S2=p.solutions[1].S, // medium
  S3=p.solutions[0].S // coarsest
     ;

  std::vector<PlotCurve> plotcrvs;

  {
    arma::mat x=vec3 ( p.solutions[2].deltax, p.solutions[1].deltax, p.solutions[0].deltax );
    arma::mat y=vec3 ( p.solutions[2].S, p.solutions[1].S, p.solutions[0].S );

    plotcrvs.push_back
    ( PlotCurve ( arma::mat ( join_rows ( x,y ) ), p.name, str ( format ( "w lp lt 1 t '%s vs. refinement parameter'" ) %p.name ) ) );
  }

  double
  epsilon21=S2-S1,
  epsilon32=S3-S2
            ;

  {
    arma::mat tabdata;
    tabdata
        << 1 << S1 << epsilon21 << 1e2*epsilon21/S1 << arma::endr
        << 2 << S2 << epsilon32 << 1e2*epsilon32/S2 << arma::endr
        << 3 << S3 << NAN << NAN << arma::endr;

    results->insert
    (
      "tableDataSummary",
      new TabularResult
      (
        boost::assign::list_of
        ( "Grid $i$ (1=finest)" )
        ( "Solution ($S_i$)" )
        ( "Diff. ($\\epsilon_{i+1,i}=S_{i+1}-S_i$)" )
        ( "$\\epsilon_{i+1,i}$ / \\%" )
        ,
        tabdata,
        "The following table repeats the input data for the convergence analysis of the solution quantity "+p.name+".", "", ""
      )
    ).setOrder ( so.next() );
  }

  double R, ep, delta, c;

  R=epsilon21/ ( 1e-10+epsilon32 );

  if ( R<0. )
    {
      results->insert
      (
        p.name+"_R",
        new ScalarResult
        (
          R,
          "Oscillation indicator",

          "Computed as:\n"
          "$$ R = \\frac{\\epsilon_{32}}{\\epsilon_{21}} $$\n"
          "A negative value $R<0$ indicates oscillatory convergence. "
          "In this case, more than 3 refinement steps are required for error estimation. "
          "This is currently not supported in the scope of this analysis. "
          "{\\bf Thus, no error estimate could be included in this result report.}",

          ""
        )
      ).setOrder ( so.next() );

      ep=NAN;
      delta=NAN;
      c=NAN;
    }
  else if ( R>1.0 )
    {
      results->insert
      (
        p.name+"_R",
        new ScalarResult
        (
          R,
          "Oscillation indicator",

          "Computed as:\n"
          "$$ R = \\frac{\\epsilon_{32}}{\\epsilon_{21}} $$\n"
          "A value of $R>1$ indicates divergence. "
          "{\\bf There is no convergence this case and thus it is not possible to perform an error estimation.}",

          ""
        )
      ).setOrder ( so.next() );

      ep=NAN;
      delta=NAN;
      c=NAN;
    }
  else
    {

      // monotonic convergence
      results->insert
      (
        p.name+"_R",
        new ScalarResult
        (
          R,
          "Oscillation indicator",

          "Computed as:\n"
          "$$ R = \\frac{\\epsilon_{32}}{\\epsilon_{21}} $$\n"
          "A value of $0<R<1$ indicates a monotonic convergent solution which is required for performing an error analysis.",

          ""
        )
      ).setOrder ( so.next() );

      ep=log ( epsilon32/ ( 1e-10+epsilon21 ) ) /log ( r );

      delta=epsilon21 / ( pow ( r, ep )-1. );

      c = ( pow ( r, ep )-1. ) / ( pow ( r, p.p_est )-1. ); // (16)

      results->insert ( p.name+"_p", new ScalarResult (
                          ep,
                          "Order of accuracy",

                          "Computed as:\n"
                          "$$ p = \\frac{\\ln \\epsilon_{32}}{\\ln \\epsilon_{21}} $$\n",
                          "" ) )
      .setOrder ( so.next() );

      results->insert ( p.name+"_delta", new ScalarResult (
                          delta,
                          "Error estimate of Richardson extrapolation",

                          "Computed as:\n"
                          " $$ \\delta^* = \\frac{ \\epsilon_{21} } {r^p - 1 } $$\n",
                          p.yunit ) )
      .setOrder ( so.next() );

      results->insert ( p.name+"_C", new ScalarResult (
                          c,
                          "Correction factor",

                          "Computed as:\n"
                          "$$ C = \\frac {r^p -1} {r^{p_{est}}-1} $$\n"
                          + str ( format ( "The estimated order of convergence $p_{est}=%.2f$ is used here." ) % p.p_est ),
                          "" ) )
      .setOrder ( so.next() );

      {
        boost::shared_ptr<ResultSection> subsection
        (
          new ResultSection
          (
            "Case of Lacking Confidence: Uncertainty Estimation",

            str ( format (
                    "If the correction factor $C$ is not close to 1, i.e. $|1-C|$ is large (here $|1-C|=%.2f$),"
                    " the result should not be corrected and only the error is estimated."
                  ) % fabs ( 1.-c ) )
          )
        );

        // too far, no confidence
        double U=fabs ( c*delta )+fabs ( ( 1.-c ) *delta );
        subsection->insert ( p.name+"_U", new ScalarResult (
                               U,
                               "Error estimate of the (uncorrected) result",

                               "Computed as:\n"
                               "$$ U = |C \\delta^*| + |(1-C) \\delta^* |$$\n",
                               p.yunit ) )
        .setOrder ( 1 );

        arma::mat xyz;
        xyz << p.solutions[2].deltax << p.solutions[2].S << U << arma::endr;
        plotcrvs.push_back ( PlotCurve ( xyz, "erroruncorrected", "w errorlines lt 1 lc 3 lw 2 t 'Error estimate (uncorrected)'" ) );

        results->insert ( "uncorrected", subsection ) .setOrder ( so.next() );
      }

      {
        boost::shared_ptr<ResultSection> subsection
        (
          new ResultSection
          (
            "Case of Confidence: Corrected Result",

            str ( format (
                    "If the correction factor $C$ is close to 1, i.e. $|1-C|$ is sufficiently small (here $|1-C|=%.2f$),"
                    " the result may be corrected."
                    "\n\n"
                    "Please note, that since the variability of the order of accuracy cannot be determined from solutions on three grids,"
                    " confidence is difficult to establish."
                    " It requires experience from a large number of verification and validation studies."
                  ) % fabs ( 1.-c ) )
          )
        );

        // sufficiently close => confidence
        double deltaG=c*delta;
        double U=fabs ( ( 1.-c ) *delta );
        double qcorr=S1-deltaG;

        subsection->insert ( p.name+"_UC", new ScalarResult (
                               U,
                               "Error estimate of the corrected result",

                               "Computed as:\n"
                               "$$ U_C = |(1-C) \\delta^*| $$\n",
                               p.yunit ) )
        .setOrder ( 1 );

        subsection->insert ( p.name+"_deltaG", new ScalarResult (
                               deltaG,
                               "Solution correction with sign and value",

                               "Computed as:\n"
                               "$$ \\delta_G = C \\delta^* $$\n",
                               p.yunit ) )
        .setOrder ( 2 );

        subsection->insert ( p.name+"_SC", new ScalarResult (
                               qcorr,
                               "Corrected result",

                               "Computed as:\n"
                               "$$ S_C = S_1-\\delta_G$$\n", p.yunit ) )
        .setOrder ( 3 );

        arma::mat xyz;
        xyz << 1.05*p.solutions[2].deltax << qcorr << U << arma::endr;
        plotcrvs.push_back ( PlotCurve ( xyz, "corrected", "w errorlines lt 2 lc 7 lw 2 t 'Corrected solution and error estimate'" ) );

        results->insert ( "corrected", subsection ) .setOrder ( so.next() );
      }

    }

  // include a plot of the iteration steps
  addPlot
  (
    results, executionPath(),
    "chartConvergence"+p.name,
    "Refinement parameter "+p.xname,
    p.name+" ["+p.yunit+"]",
    plotcrvs,
    "Convergence of quantity "+p.name+
    " (X coordinate of corrected solution is shifted by an arbitrary small distance to the right for readability)"
  ).setOrder ( 99 );



  return results;
}

}
