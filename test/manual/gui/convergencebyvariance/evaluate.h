#ifndef CONVERGENCEBYVARIANCE_EVALUATE_H
#define CONVERGENCEBYVARIANCE_EVALUATE_H

#include "base/linearalgebra.h"

#include <vector>
#include <string>

struct CaseData;

/**
 * Result of evaluating ConvergenceByVariance for one series
 * within a time window.
 */
struct ColumnResult
{
  arma::mat t_cov;        // n x 2: time, CoV history
  arma::uvec window;      // n: effective trailing window size (number of samples) per sample
  arma::vec windowSpan;   // n: time span covered by the trailing window per sample
  double finalCoV = 0.0;  // last CoV value (0 if no samples)
  arma::uword nSamples = 0;
  double t0 = 0.0, t1 = 0.0; // effectively used time window
};

/**
 * Feeds the rows of tx (time, value) with t0 <= t <= t1
 * sample by sample into insight::ConvergenceByVariance
 * and returns the convergence measure history.
 */
ColumnResult evaluate(
    const arma::mat& tx,
    double fraction, double t0, double t1 );

/**
 * Evaluates all series of a case.
 * t0, t1: time window, NaN = time range of the respective series.
 */
std::vector<ColumnResult> evaluate(
    const CaseData& cd,
    double fraction, double t0, double t1 );

/**
 * convergence criterion
 */
enum class WindowUnit { Samples, Time };

struct Criterion
{
  double threshold = 1e-2;      // CoV has to be below this value
  // ... and the trailing window has to contain at least minWindow samples (unit==Samples)
  // or to cover at least a time span of minWindow (unit==Time, additionally >=2 samples)
  WindowUnit unit = WindowUnit::Samples;
  double minWindow = 50;
};

/**
 * trailing window of sample k fulfills the min. window size requirement
 */
bool windowLargeEnough(const ColumnResult& r, arma::uword k, const Criterion& c);

/**
 * trailing window at the final sample is too small (false, if no samples)
 */
bool windowTooSmall(const ColumnResult& r, const Criterion& c);

/**
 * e.g. "50 samples" or "time span 0.5"
 */
std::string minWindowText(const Criterion& c);

/**
 * sample k of the result fulfills the criterion
 */
bool converged(const ColumnResult& r, arma::uword k, const Criterion& c);

/**
 * final sample fulfills the criterion
 */
bool converged(const ColumnResult& r, const Criterion& c);

/**
 * Converged:        criterion fulfilled at the end of the time window
 * ConvergedEarlier: not fulfilled at the end, but at some earlier time
 * NotConverged:     never fulfilled
 */
enum class SeriesStatus { Converged, ConvergedEarlier, NotConverged };

SeriesStatus seriesStatus(const ColumnResult& r, const Criterion& c);

/**
 * Converged:        all selected series are converged at the end
 * ConvergedEarlier: not converged at the end, but all selected series were
 *                   converged simultaneously at some time (see firstConvergedTime)
 * NotConverged:     never all converged simultaneously
 * NoSelection:      no series selected
 * Deselected series are ignored.
 */
enum class CaseStatus { Converged, ConvergedEarlier, NotConverged, NoSelection };

CaseStatus caseStatus(const CaseData& cd, const std::vector<ColumnResult>& r, const Criterion& c);

/**
 * earliest time, at which all selected series of the case
 * fulfilled the criterion simultaneously.
 * Between samples, a series keeps its last state,
 * before its first evaluated sample, it is considered not converged.
 * Returns NaN, if this never happened (or no series is selected).
 */
double firstConvergedTime(const CaseData& cd, const std::vector<ColumnResult>& r, const Criterion& c);

#endif
