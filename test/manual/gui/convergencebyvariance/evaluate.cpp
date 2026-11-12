#include "evaluate.h"
#include "casereader.h"

#include <cmath>
#include <algorithm>
#include <limits>
#include <tuple>
#include <sstream>

ColumnResult evaluate(
    const arma::mat& tx,
    double fraction, double t0, double t1 )
{
  ColumnResult r;
  r.t0 = t0;
  r.t1 = t1;

  arma::uvec rows = arma::find( (tx.col(0) >= t0) % (tx.col(0) <= t1) );

  insight::ConvergenceByVariance cbv(fraction);
  for (arma::uword i=0; i<rows.n_elem; ++i)
  {
    cbv( arma::rowvec{ tx(rows(i), 1) } );
  }

  r.nSamples = rows.n_elem;

  // effective window size, as used in ConvergenceByVariance:
  // max(2, round(fraction*n)), limited by the number of samples n
  r.window.set_size(r.nSamples);
  r.windowSpan.set_size(r.nSamples);
  for (arma::uword k=0; k<r.nSamples; ++k)
  {
    arma::uword n=k+1;
    r.window(k) = std::min<arma::uword>(
        n, std::max<arma::uword>(2, arma::uword(std::round(fraction*n))) );
    r.windowSpan(k) = tx(rows(k),0) - tx(rows(k+1-r.window(k)),0);
  }

  if (r.nSamples>0)
  {
    r.t_cov = arma::join_horiz(
        tx.submat(rows, arma::uvec{0}),
        cbv.convergenceMeasure() );
    r.finalCoV = r.t_cov(r.t_cov.n_rows-1, 1);
  }
  return r;
}


std::vector<ColumnResult> evaluate(
    const CaseData& cd,
    double fraction, double t0, double t1 )
{
  std::vector<ColumnResult> res;
  for (const auto& s: cd.series)
  {
    res.push_back( evaluate(
        s.tx, fraction,
        std::isnan(t0) ? s.tx.col(0).min() : t0,
        std::isnan(t1) ? s.tx.col(0).max() : t1 ) );
  }
  return res;
}


bool converged(const ColumnResult& r, arma::uword k, const Criterion& c)
{
  return k<r.nSamples
         && r.t_cov(k,1) < c.threshold
         && windowLargeEnough(r, k, c);
}


bool windowLargeEnough(const ColumnResult& r, arma::uword k, const Criterion& c)
{
  if (k>=r.nSamples) return false;
  if (c.unit==WindowUnit::Samples)
    return double(r.window(k)) >= c.minWindow;
  else
    return r.window(k)>=2 && r.windowSpan(k) >= c.minWindow;
}


bool windowTooSmall(const ColumnResult& r, const Criterion& c)
{
  return r.nSamples>0 && !windowLargeEnough(r, r.nSamples-1, c);
}


std::string minWindowText(const Criterion& c)
{
  std::ostringstream os;
  if (c.unit==WindowUnit::Samples)
    os << c.minWindow << " samples";
  else
    os << "time span " << c.minWindow;
  return os.str();
}


bool converged(const ColumnResult& r, const Criterion& c)
{
  return r.nSamples>0 && converged(r, r.nSamples-1, c);
}


SeriesStatus seriesStatus(const ColumnResult& r, const Criterion& c)
{
  if (converged(r, c))
    return SeriesStatus::Converged;
  for (arma::uword k=0; k<r.nSamples; ++k)
    if (converged(r, k, c))
      return SeriesStatus::ConvergedEarlier;
  return SeriesStatus::NotConverged;
}


CaseStatus caseStatus(const CaseData& cd, const std::vector<ColumnResult>& r, const Criterion& c)
{
  bool anySelected=false, allConverged=true;
  for (size_t i=0; i<cd.series.size() && i<r.size(); ++i)
  {
    if (!cd.series[i].selected) continue;
    anySelected=true;
    if (!converged(r[i], c))
      allConverged=false;
  }
  if (!anySelected)
    return CaseStatus::NoSelection;
  if (allConverged)
    return CaseStatus::Converged;
  return std::isnan(firstConvergedTime(cd, r, c)) ?
             CaseStatus::NotConverged : CaseStatus::ConvergedEarlier;
}



double firstConvergedTime(const CaseData& cd, const std::vector<ColumnResult>& r, const Criterion& crit)
{
  const double never = std::numeric_limits<double>::quiet_NaN();

  // events: (time, series index, criterion fulfilled)
  std::vector<std::tuple<double, size_t, bool> > events;
  size_t nSel=0;
  for (size_t i=0; i<cd.series.size() && i<r.size(); ++i)
  {
    if (!cd.series[i].selected) continue;
    ++nSel;
    const auto& tc = r[i].t_cov;
    if (tc.n_rows==0) return never; // no samples => never converged
    for (arma::uword k=0; k<tc.n_rows; ++k)
      events.emplace_back(tc(k,0), i, converged(r[i], k, crit));
  }
  if (nSel==0) return never;

  std::stable_sort(events.begin(), events.end(),
            [](const auto& a, const auto& b) { return std::get<0>(a)<std::get<0>(b); });

  std::vector<char> isConv(cd.series.size(), 0);
  size_t nConv=0;
  for (size_t e=0; e<events.size(); )
  {
    double t=std::get<0>(events[e]);
    // apply all samples at this time before checking
    for (; e<events.size() && std::get<0>(events[e])==t; ++e)
    {
      size_t i=std::get<1>(events[e]);
      char c = std::get<2>(events[e]);
      if (c && !isConv[i]) ++nConv;
      if (!c && isConv[i]) --nConv;
      isConv[i]=c;
    }
    if (nConv==nSel) return t;
  }
  return never;
}
