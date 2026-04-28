#include "base/linearalgebra.h"
#include "base/exception.h"

#include <cmath>
#include <iostream>

using namespace insight;

// -----------------------------------------------------------------------
// Helper: compute expected CoV analytically for a contiguous integer ramp
// values 1..n, last W values, using Bessel-corrected stddev.
// -----------------------------------------------------------------------
static double rampCoV(int n, int W)
{
    // last W values: (n-W+1), ..., n
    double mu = (static_cast<double>(n - W + 1) + n) / 2.0;
    // sum of squared deviations from mean for an arithmetic sequence of length W:
    // deviations are -(W-1)/2, ..., (W-1)/2  (spacing 1)
    double sum_sq = 0.0;
    for (int k = 0; k < W; ++k)
    {
        double d = (n - W + 1 + k) - mu;
        sum_sq += d * d;
    }
    double sig = std::sqrt(sum_sq / (W - 1));
    return sig / std::abs(mu);
}

int main()
{
    try
    {
        const double tol = 1e-10;

        // -------------------------------------------------------------------
        // 1. Batch: output shape equals input shape
        // -------------------------------------------------------------------
        {
            arma::mat values = arma::randn(40, 3);
            arma::mat cov = convergenceByVariance(values, 0.2);
            insight::assertion(
                cov.n_rows == 40 && cov.n_cols == 3,
                "batch: output shape does not match input shape");
            std::cout << "PASS  1: batch output shape" << std::endl;
        }

        // -------------------------------------------------------------------
        // 2. Batch: constant signal → CoV = 0 once window is full
        // -------------------------------------------------------------------
        {
            const int n = 50;
            arma::mat values = arma::ones(n, 1) * 3.14;
            arma::mat cov = convergenceByVariance(values, 0.1);

            const arma::uword W =
                std::max<arma::uword>(2, static_cast<arma::uword>(std::round(0.1 * n)));
            for (arma::uword i = W - 1; i < static_cast<arma::uword>(n); ++i)
            {
                insight::assertion(
                    std::abs(cov(i, 0)) < tol,
                    "batch: CoV not zero for constant signal");
            }
            std::cout << "PASS  2: batch constant signal" << std::endl;
        }

        // -------------------------------------------------------------------
        // 3. Batch: oscillating signal → CoV > 0 once window is full
        // -------------------------------------------------------------------
        {
            const int n = 100;
            arma::mat values(n, 1);
            for (int i = 0; i < n; ++i)
                values(i, 0) = std::sin(i * 0.3) + 5.0; // positive mean, oscillating

            arma::mat cov = convergenceByVariance(values, 0.1);

            const arma::uword W =
                std::max<arma::uword>(2, static_cast<arma::uword>(std::round(0.1 * n)));
            double max_cov = arma::as_scalar(arma::max(arma::vectorise(cov.rows(W - 1, n - 1))));
            insight::assertion(max_cov > 0.0, "batch: expected nonzero CoV for oscillating signal");
            std::cout << "PASS  3: batch oscillating signal" << std::endl;
        }

        // -------------------------------------------------------------------
        // 4. Class: constant signal → CoV = 0 from the second sample onward
        // -------------------------------------------------------------------
        {
            ConvergenceByVariance cbv(0.1);
            const int n = 60;
            for (int i = 0; i < n; ++i)
            {
                arma::rowvec s = {7.5};
                arma::rowvec cov = cbv(s);
                if (i >= 1)
                {
                    insight::assertion(
                        std::abs(cov(0)) < tol,
                        "class: CoV not zero for constant signal");
                }
            }
            insight::assertion(
                cbv.values().n_rows == static_cast<arma::uword>(n),
                "class: values() history has wrong size");
            insight::assertion(
                cbv.convergenceMeasure().n_rows == static_cast<arma::uword>(n),
                "class: convergenceMeasure() history has wrong size");
            std::cout << "PASS  4: class constant signal" << std::endl;
        }

        // -------------------------------------------------------------------
        // 5. Class: zero-mean signal → no NaN or Inf (exercises range fallback)
        // -------------------------------------------------------------------
        {
            ConvergenceByVariance cbv(0.1);
            const int n = 80;
            for (int i = 0; i < n; ++i)
            {
                arma::rowvec s = {std::sin(i * 0.5)}; // mean ≈ 0
                arma::rowvec cov = cbv(s);
                insight::assertion(
                    std::isfinite(cov(0)),
                    "class: non-finite CoV on zero-mean signal");
            }
            std::cout << "PASS  5: class zero-mean signal (range fallback, no NaN/Inf)" << std::endl;
        }

        // -------------------------------------------------------------------
        // 6. Class: multi-column — columns are treated independently
        //    col 0: constant   → CoV must be 0 once window fills
        //    col 1: oscillating + offset → CoV must be > 0
        // -------------------------------------------------------------------
        {
            ConvergenceByVariance cbv(0.1);
            const int n = 50;
            for (int i = 0; i < n; ++i)
            {
                arma::rowvec s = {3.0, std::sin(i * 0.4) + 5.0};
                cbv(s);
            }

            const arma::mat& cov = cbv.convergenceMeasure();
            insight::assertion(cov.n_cols == 2, "class: expected 2 columns in convergenceMeasure");

            // col 0 must be ~0 once window has filled
            for (arma::uword i = 1; i < static_cast<arma::uword>(n); ++i)
            {
                insight::assertion(
                    std::abs(cov(i, 0)) < tol,
                    "class: constant column CoV not zero");
            }

            // col 1 must be positive somewhere
            double max_cov1 = arma::as_scalar(arma::max(cov.col(1)));
            insight::assertion(max_cov1 > 0.0, "class: expected nonzero CoV for oscillating column");

            std::cout << "PASS  6: class multi-column independence" << std::endl;
        }

        // -------------------------------------------------------------------
        // 7. Class: analytical check of final CoV for a linear ramp
        //
        //    Signal: x(i) = i+1  for i = 0..99  (values 1..100)
        //    fraction = 0.1  →  final W = max(2, round(0.1*100)) = 10
        //    Last window: values 91..100, mean = 95.5
        //    Bessel stddev = sqrt(82.5/9)  ≈  3.02765
        //    CoV = stddev / mean           ≈  0.031706
        //
        //    This also exercises both the EXPAND path (W grows 2→3→…→10 at
        //    n = 25, 35, 45, 55, 65, 75, 85, 95) and the SLIDE path (all other steps).
        // -------------------------------------------------------------------
        {
            ConvergenceByVariance cbv(0.1);
            const int n = 100;
            for (int i = 0; i < n; ++i)
            {
                arma::rowvec s = {static_cast<double>(i + 1)};
                cbv(s);
            }

            const int W_final = static_cast<int>(
                std::max<arma::uword>(2, static_cast<arma::uword>(std::round(0.1 * n))));
            const double expected = rampCoV(n, W_final);
            const double actual   = cbv.convergenceMeasure()(n - 1, 0);

            insight::assertion(
                std::abs(actual - expected) < 1e-9,
                "class: final CoV for linear ramp does not match analytical value");

            std::cout << "PASS  7: class linear ramp analytical check"
                      << "  (CoV = " << actual << ", expected " << expected << ")" << std::endl;
        }

        // -------------------------------------------------------------------
        // 8. Class: convergence measure decreases as signal settles
        //
        //    First half: oscillating (sin with amplitude 1, mean 5)
        //    Second half: constant at 5  →  CoV must be lower in second half
        // -------------------------------------------------------------------
        {
            ConvergenceByVariance cbv(0.1);
            const int n = 200;
            for (int i = 0; i < n; ++i)
            {
                double v = (i < n / 2)
                           ? std::sin(i * 0.4) + 5.0   // oscillating
                           : 5.0;                       // settled
                arma::rowvec s = {v};
                cbv(s);
            }

            const arma::mat& cov = cbv.convergenceMeasure();

            // Average CoV in the first quarter vs the last quarter
            const arma::uword q = n / 4;
            double mean_early = arma::mean(arma::vectorise(cov.rows(q, 2 * q - 1)));
            double mean_late  = arma::mean(arma::vectorise(cov.rows(3 * q, n - 1)));

            insight::assertion(
                mean_late < mean_early,
                "class: convergence measure did not decrease after signal settled");

            std::cout << "PASS  8: class convergence measure decreases after signal settles"
                      << "  (early avg CoV = " << mean_early
                      << ", late avg CoV = " << mean_late << ")" << std::endl;
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
