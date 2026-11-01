#include "base/exception.h"
#include "base/patternsearch.h"
#include "base/progressdisplayer.h"
#include "base/actionprogress.h"

#include <iostream>
#include <cmath>
#include <limits>
#include <atomic>
#include <chrono>
#include <thread>

using namespace insight;


namespace {

void check(bool cond, const std::string& msg)
{
    std::cout << (cond ? "OK:     " : "FAILED: ") << msg << std::endl;
    if (!cond) throw insight::Exception("check failed: "+msg);
}


// requests a stop after a given number of evaluations
class StoppingDisplayer : public ProgressDisplayer
{
public:
    int *nEval = nullptr;
    int stopAfter = 0;

    void setActionProgressValue(const std::string &, double) override {}
    void setMessageText(const std::string &, const std::string&) override {}
    void finishActionProgress(const std::string &) override {}
    void reset() override {}
    void update(const ProgressState&) override {}
    void logMessage(const std::string&) override {}

    bool stopIsDemanded() const override
    {
        return *nEval >= stopAfter;
    }
};

}


int main(int, char*[])
{
    try
    {
        // quadratic with interior optimum
        {
            auto f = [](const arma::vec& x)
            {
                return std::pow(x(0)-0.3, 2) + 2.*std::pow(x(1)+0.2, 2);
            };
            auto r = boundedPatternSearch(
                f, arma::vec({0.8, 0.5}), arma::vec({-1., -1.}), arma::vec({1., 1.}),
                { 0.1, 1e-4, 1000 } );
            std::cout<<"interior: x="<<r.x.t()<<" f="<<r.f<<" nEval="<<r.nEvaluations<<std::endl;
            check(std::fabs(r.x(0)-0.3)<1e-3 && std::fabs(r.x(1)+0.2)<1e-3, "interior optimum found");
            check(r.f<r.f0, "objective improved");
            check(r.nEvaluations<=1000, "evaluation budget respected");
        }

        // optimum outside of the bounds: on the bound
        {
            auto f = [](const arma::vec& x)
            {
                return std::pow(x(0)-2., 2) + std::pow(x(1), 2);
            };
            auto r = boundedPatternSearch(
                f, arma::vec({0., 0.5}), arma::vec({-1., -1.}), arma::vec({1., 1.}),
                { 0.1, 1e-4, 1000 } );
            std::cout<<"bound: x="<<r.x.t()<<std::endl;
            check(std::fabs(r.x(0)-1.)<1e-9, "optimum clipped at the upper bound");
            check(std::fabs(r.x(1))<1e-3, "free variable optimized");
        }

        // start outside the bounds: clipped
        {
            auto f = [](const arma::vec& x) { return std::pow(x(0), 2); };
            auto r = boundedPatternSearch(
                f, arma::vec({5.}), arma::vec({-1.}), arma::vec({1.}),
                { 0.1, 1e-4, 1000 } );
            check(arma::all(r.x>=-1.) && arma::all(r.x<=1.), "start point clipped into the bounds");
            check(std::fabs(r.x(0))<1e-3, "optimum found from clipped start");
        }

        // infeasible region (x0+x1>0.5) and NaN
        {
            int nInf=0;
            auto f = [&](const arma::vec& x)
            {
                if (x(0)+x(1)>0.5) { ++nInf; return std::numeric_limits<double>::infinity(); }
                if (x(0)<-0.9) return std::numeric_limits<double>::quiet_NaN();
                return std::pow(x(0)-1., 2) + std::pow(x(1)-1., 2);
            };
            auto r = boundedPatternSearch(
                f, arma::vec({0., 0.}), arma::vec({-1., -1.}), arma::vec({1., 1.}),
                { 0.1, 1e-4, 1000 } );
            std::cout<<"infeasible: x="<<r.x.t()<<" f="<<r.f<<" (infeasible evaluations: "<<nInf<<")"<<std::endl;
            check(r.x(0)+r.x(1)<=0.5+1e-12, "result is feasible");
            check(std::isfinite(r.f), "finite objective at the result");
            check(std::fabs(r.x(0)+r.x(1)-0.5)<1e-2, "result at the feasibility limit");
        }

        // parallel poll
        {
            auto f = [](const arma::vec& x)
            {
                return std::pow(x(0)-0.3, 2) + 2.*std::pow(x(1)+0.2, 2);
            };
            PatternSearchSettings ps{ 0.1, 1e-4, 1000 };
            ps.nThreads = 4;
            auto r = boundedPatternSearch(
                f, arma::vec({0.8, 0.5}), arma::vec({-1., -1.}), arma::vec({1., 1.}), ps );
            std::cout<<"parallel interior: x="<<r.x.t()<<" nEval="<<r.nEvaluations<<std::endl;
            check(std::fabs(r.x(0)-0.3)<1e-3 && std::fabs(r.x(1)+0.2)<1e-3, "parallel: interior optimum found");
            check(r.nEvaluations<=1000, "parallel: evaluation budget respected");

            auto fb = [](const arma::vec& x)
            {
                return std::pow(x(0)-2., 2) + std::pow(x(1), 2);
            };
            auto rb = boundedPatternSearch(
                fb, arma::vec({0., 0.5}), arma::vec({-1., -1.}), arma::vec({1., 1.}), ps );
            check(std::fabs(rb.x(0)-1.)<1e-9 && std::fabs(rb.x(1))<1e-3, "parallel: optimum clipped at the bound");

            auto fi = [](const arma::vec& x)
            {
                if (x(0)+x(1)>0.5) return std::numeric_limits<double>::infinity();
                if (x(0)<-0.9) throw insight::Exception("failing evaluation");
                return std::pow(x(0)-1., 2) + std::pow(x(1)-1., 2);
            };
            auto ri = boundedPatternSearch(
                fi, arma::vec({0., 0.}), arma::vec({-1., -1.}), arma::vec({1., 1.}), ps );
            check(ri.x(0)+ri.x(1)<=0.5+1e-12 && std::isfinite(ri.f), "parallel: feasible result, exceptions are infeasible");

            ps.maxEvaluations = 13;
            auto rl = boundedPatternSearch(
                f, arma::vec({0.8, 0.5}), arma::vec({-1., -1.}), arma::vec({1., 1.}), ps );
            check(rl.nEvaluations<=13, "parallel: small evaluation budget respected");
        }

        // evaluations run concurrently
        {
            std::atomic<int> active{0}, maxActive{0};
            auto f = [&](const arma::vec& x)
            {
                int a = ++active;
                int m = maxActive;
                while (a>m && !maxActive.compare_exchange_weak(m, a)) {}
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                --active;
                return arma::accu(arma::square(x));
            };
            std::vector<arma::vec> pts(8, arma::vec({1., 2.}));
            auto v = evaluateInParallel(f, pts, 4);
            std::cout<<"max. concurrent evaluations: "<<maxActive<<std::endl;
            check(maxActive>1 && maxActive<=4, "objective evaluated concurrently, at most nThreads at a time");
            check(v.size()==8 && std::fabs(v[7]-5.)<1e-12, "values of the parallel evaluation");
        }

        // stop: best point found so far
        {
            int nEval=0;
            double fBest = std::numeric_limits<double>::infinity();
            auto f = [&](const arma::vec& x)
            {
                ++nEval;
                double v = arma::accu(arma::square(x-0.5));
                fBest = std::min(fBest, v);
                return v;
            };
            StoppingDisplayer d;
            d.nEval = &nEval;
            d.stopAfter = 7;
            auto ap = d.forkNewAction(1, "search");
            auto r = boundedPatternSearch(
                f, arma::vec({0., 0., 0.}), arma::vec({-1., -1., -1.}), arma::vec({1., 1., 1.}),
                { 0.1, 1e-4, 1000 }, ap.get() );
            std::cout<<"stop: nEval="<<r.nEvaluations<<" f="<<r.f<<std::endl;
            check(r.stopped, "stop reported");
            check(r.nEvaluations==7, "stopped after the requested number of evaluations");
            check(r.f==fBest && r.f<r.f0, "best point found so far returned");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
