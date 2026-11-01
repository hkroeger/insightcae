#ifndef INSIGHT_PATTERNSEARCH_H
#define INSIGHT_PATTERNSEARCH_H

#include <armadillo>
#include <functional>

namespace insight {

class ActionProgress;




struct PatternSearchSettings
{
    double initialStep = 0.1;   //!< initial step, relative to the range of each variable
    double minStep = 1e-3;      //!< convergence: step (relative to the range) below which the search stops
    int maxEvaluations = 500;   //!< max. number of objective evaluations
    int nThreads = 1;           //!< number of objective evaluations in parallel (0: number of CPU cores).
                                //!< 1: sequential search, which accepts improvements immediately.
                                //!< >1: all poll points of an iteration are evaluated in parallel.
};




struct PatternSearchResult
{
    arma::vec x;                //!< best point found
    double f;                   //!< objective at x
    double f0;                  //!< objective at the start point
    int nEvaluations = 0;
    bool stopped = false;       //!< stopped by the user (x is the best point found so far)
};




/**
 * evaluates f at all points in parallel threads (nThreads 0: number of CPU cores).
 * Exceptions of f and NaN yield +inf. Warnings issued in the threads are forwarded
 * to the warning dispatcher of the calling thread.
 * @param skip
 * if given and true, the remaining points are not evaluated (their value is +inf)
 * @param onEvaluated
 * called after each evaluation with the index of the point (from the worker threads, serialized)
 */
std::vector<double> evaluateInParallel(
    const std::function<double(const arma::vec&)>& f,
    const std::vector<arma::vec>& points,
    int nThreads,
    const std::function<bool()>& skip = std::function<bool()>(),
    const std::function<void(size_t, double)>& onEvaluated = std::function<void(size_t, double)>() );




/**
 * @brief boundedPatternSearch
 * minimizes f(x) for lo<=x<=hi with a compass search (derivative free):
 * per sweep, each variable is changed by +/-step (clipped to the bounds),
 * improvements are accepted immediately and a successful direction is extended once more.
 * If a sweep yields no improvement, the step is halved.
 * With nThreads>1, all poll points (+/-step in each variable) of an iteration are evaluated in parallel,
 * together with the point combining all improving steps, and the best is accepted.
 * Robust against noisy or slightly discontinuous objectives, the best point found so far is always kept.
 * @param f
 * objective, may return +inf (or NaN) for infeasible points.
 * Must be thread-safe, if settings.nThreads!=1.
 * @param progress
 * progress reporting, the search can be stopped via the stop signal (keeping the best point found so far)
 */
PatternSearchResult boundedPatternSearch(
    const std::function<double(const arma::vec&)>& f,
    const arma::vec& x0,
    const arma::vec& lo,
    const arma::vec& hi,
    const PatternSearchSettings& settings = PatternSearchSettings(),
    ActionProgress* progress = nullptr );




} // namespace insight

#endif // INSIGHT_PATTERNSEARCH_H
