#include "patternsearch.h"

#include "base/exception.h"
#include "base/actionprogress.h"
#include "base/warningdispatcher.h"

#include "boost/format.hpp"
#include "boost/thread.hpp"

#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>

namespace insight {




std::vector<double> evaluateInParallel(
    const std::function<double(const arma::vec&)>& f,
    const std::vector<arma::vec>& points,
    int nThreads,
    const std::function<bool()>& skip,
    const std::function<void(size_t, double)>& onEvaluated )
{
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<double> values(points.size(), inf);
    if (points.empty()) return values;

    if (nThreads<=0)
        nThreads = std::max<int>(1, boost::thread::hardware_concurrency());
    nThreads = std::min<int>(nThreads, int(points.size()));

    std::atomic<size_t> next{0};
    std::mutex cbMutex;
    auto* mainThreadWD = &WarningDispatcher::getCurrent();

    auto work = [&](bool inMainThread)
    {
        if (!inMainThread)
            WarningDispatcher::getCurrent().setSuperDispatcher(mainThreadWD);
        for (size_t i=next++; i<points.size(); i=next++)
        {
            if (skip && skip()) continue;
            double v = inf;
            try
            {
                v = f(points[i]);
                if (std::isnan(v)) v = inf;
            }
            catch (...)
            {
                v = inf;
            }
            values[i] = v;
            if (onEvaluated)
            {
                std::lock_guard<std::mutex> l(cbMutex);
                onEvaluated(i, v);
            }
        }
    };

    std::vector<std::unique_ptr<boost::thread> > workers;
    for (int t=1; t<nThreads; ++t)
        workers.push_back(std::make_unique<boost::thread>([&]() { work(false); }));
    work(true);
    for (auto& w: workers) w->join();

    return values;
}




PatternSearchResult boundedPatternSearch(
    const std::function<double(const arma::vec&)>& f,
    const arma::vec& x0,
    const arma::vec& lo,
    const arma::vec& hi,
    const PatternSearchSettings& settings,
    ActionProgress* progress )
{
    const arma::uword n = x0.n_elem;
    insight::assertion(
        lo.n_elem==n && hi.n_elem==n,
        "inconsistent number of bounds (%d, %d) and variables (%d)",
        int(lo.n_elem), int(hi.n_elem), int(n) );
    insight::assertion(
        arma::all(hi>=lo),
        "upper bounds must not be smaller than the lower bounds" );

    const arma::vec range = hi-lo;

    PatternSearchResult res;
    res.x = arma::min(arma::max(x0, lo), hi);
    res.nEvaluations = 0;

    auto eval = [&](const arma::vec& x)
    {
        ++res.nEvaluations;
        double v = f(x);
        if (std::isnan(v)) v = std::numeric_limits<double>::infinity();
        return v;
    };

    auto stopDemanded = [&]()
    {
        if (progress && progress->stopIsDemanded())
            res.stopped = true;
        return res.stopped;
    };

    auto budgetLeft = [&]()
    {
        return res.nEvaluations < settings.maxEvaluations;
    };

    res.f0 = res.f = eval(res.x);

    if (progress)
    {
        progress->setNSteps(std::max(1, settings.maxEvaluations));
        progress->message(str(boost::format(
            "pattern search over %d variables: initial objective %g") % n % res.f ));
    }

    // try x+dir*s*range(i), return true, if improved
    auto tryStep = [&](arma::uword i, double ds)
    {
        if (range(i)<=0.) return false;
        arma::vec xt = res.x;
        xt(i) = std::min(hi(i), std::max(lo(i), xt(i)+ds*range(i)));
        if (std::fabs(xt(i)-res.x(i)) < 1e-12*range(i)) return false; // at the bound

        double ft = eval(xt);
        if (progress) progress->stepTo(res.nEvaluations);
        if (ft < res.f)
        {
            res.x = xt;
            res.f = ft;
            return true;
        }
        return false;
    };

    // point with x(i) changed by ds (relative), clipped. Returns false, if at the bound.
    auto moved = [&](const arma::vec& x, arma::uword i, double ds, arma::vec& xt)
    {
        if (range(i)<=0.) return false;
        xt = x;
        xt(i) = std::min(hi(i), std::max(lo(i), xt(i)+ds*range(i)));
        return std::fabs(xt(i)-x(i)) >= 1e-12*range(i);
    };

    // evaluate a batch in parallel, return the index of the best improving point (-1: none)
    auto evaluateBatch = [&](std::vector<arma::vec> pts, std::vector<double>& values)
    {
        int nLeft = settings.maxEvaluations-res.nEvaluations;
        if (int(pts.size())>nLeft) pts.resize(std::max(0, nLeft));
        values = evaluateInParallel(
            f, pts, settings.nThreads,
            [&]() { return progress && progress->stopIsDemanded(); },
            [&](size_t, double)
            {
                ++res.nEvaluations;
                if (progress) progress->stepTo(res.nEvaluations);
            } );
        values.resize(pts.size());
        int best=-1;
        for (size_t k=0; k<values.size(); ++k)
            if (values[k]<res.f && (best<0 || values[k]<values[best])) best=int(k);
        if (best>=0)
        {
            res.x = pts[best];
            res.f = values[best];
        }
        return best;
    };

    double step = settings.initialStep;
    bool parallel = settings.nThreads!=1;
    while ( parallel && step >= settings.minStep && budgetLeft() && !stopDemanded() )
    {
        // complete poll around the current point
        std::vector<arma::vec> pts;
        std::vector<std::pair<arma::uword,double> > moves;
        arma::vec xt;
        for (arma::uword i=0; i<n; ++i)
            for (double dir: {1., -1.})
                if (moved(res.x, i, dir*step, xt))
                {
                    pts.push_back(xt);
                    moves.push_back({i, dir*step});
                }

        const arma::vec xc = res.x;
        const double fc = res.f;
        std::vector<double> values;
        int best = evaluateBatch(pts, values);

        if (best>=0 && budgetLeft() && !stopDemanded())
        {
            // combine all improving moves (the better sign per variable),
            // and extend the best move once more
            arma::vec xComb = xc;
            int nComb = 0;
            for (arma::uword i=0; i<n; ++i)
            {
                int bi=-1;
                for (size_t k=0; k<moves.size(); ++k)
                    if (moves[k].first==i && values[k]<fc && (bi<0 || values[k]<values[bi])) bi=int(k);
                if (bi>=0)
                {
                    xComb(i) = pts[bi](i);
                    ++nComb;
                }
            }
            std::vector<arma::vec> pts2;
            if (nComb>1) pts2.push_back(xComb);
            if (moved(res.x, moves[best].first, moves[best].second, xt))
                pts2.push_back(xt);
            evaluateBatch(pts2, values);
        }

        if (progress)
            progress->message(str(boost::format(
                "pattern search: %d evaluations, step %g, objective %g (initial %g)")
                % res.nEvaluations % step % res.f % res.f0 ));

        if (best<0) step *= 0.5;
    }

    while ( !parallel && step >= settings.minStep && budgetLeft() && !stopDemanded() )
    {
        bool improved = false;
        for (arma::uword i=0; i<n && budgetLeft() && !stopDemanded(); ++i)
        {
            for (double dir: {1., -1.})
            {
                if (!budgetLeft() || stopDemanded()) break;
                if (tryStep(i, dir*step))
                {
                    improved = true;
                    // extend the successful direction once more
                    if (budgetLeft() && !stopDemanded())
                        tryStep(i, dir*step);
                    break;
                }
            }
        }

        if (progress)
            progress->message(str(boost::format(
                "pattern search: %d evaluations, step %g, objective %g (initial %g)")
                % res.nEvaluations % step % res.f % res.f0 ));

        if (!improved) step *= 0.5;
    }

    stopDemanded();

    if (progress)
    {
        if (res.stopped)
            progress->message("pattern search stopped by user, keeping the best point found so far");
        else if (step < settings.minStep)
            progress->message("pattern search converged");
        else
            progress->message("pattern search: max. number of evaluations reached");
        progress->completed();
    }

    return res;
}




} // namespace insight
