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
 */

#include "cuttingstockoptimizer.h"

#include "cadfeature.h"
#include "geotest.h"
#include "parser.h"
#include "parameterlisthash.h"

#include "GeomAbs_CurveType.hxx"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <vector>

namespace qi  = boost::spirit::qi;
namespace phx = boost::phoenix;

namespace insight {
namespace cad {

// ============================================================================
//  Internal helpers: LP solver and knapsack DP
// ============================================================================
namespace {

// ---------------------------------------------------------------------------
// Solve LP relaxation of the cutting-stock master problem.
//
//   min  sum_j x_j
//   s.t. sum_j A[i][j] * x_j  >= b[i]   for all i
//        x_j >= 0
//
// Method: big-M full-tableau simplex with surplus + artificial variables.
//
//   Variables: x_0..x_{n-1}  (pattern usage)
//              s_0..s_{m-1}  (surplus,    col n+i)
//              r_0..r_{m-1}  (artificial, col n+m+i)
//   Objective row (index m): reduced costs.
//
// Returns true on success; fills x (primal) and y (dual = shadow prices of
// the >= constraints, extracted as reduced costs of the surplus variables).
// ---------------------------------------------------------------------------
// c[j] = objective coefficient for pattern j (e.g. bar length of the stock type used)
bool solveLPRelaxation(
    const std::vector<std::vector<double>>& A,   // m x n
    const std::vector<double>&              b,
    const std::vector<double>&              c,   // n objective coefficients
    std::vector<double>&                    x,
    std::vector<double>&                    y)
{
    const int m      = static_cast<int>(b.size());
    const int n      = static_cast<int>(A[0].size());
    const int nvars  = n + 2 * m;   // patterns + surplus + artificials
    const int RHS    = nvars;        // index of the RHS column
    const double BIG_M = 1.0e9;

    // Tableau: (m+1) rows x (nvars+1) cols
    std::vector<std::vector<double>> T(m + 1, std::vector<double>(nvars + 1, 0.0));

    // Fill constraint rows: A*x - s + r = b
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) T[i][j]       =  A[i][j];
        T[i][n + i]     = -1.0;   // surplus s_i
        T[i][n + m + i] =  1.0;   // artificial r_i
        T[i][RHS]        =  b[i];
    }

    // Objective row: c[j] for x_j, 0 for s_i, BIG_M for r_i
    for (int j = 0; j < n; ++j)  T[m][j]         = c[j];
    for (int i = 0; i < m; ++i)  T[m][n + m + i] = BIG_M;

    // Eliminate basic artificials from objective row (they start basic with value b[i])
    for (int i = 0; i < m; ++i) {
        const double coeff = T[m][n + m + i];   // = BIG_M
        for (int j = 0; j <= nvars; ++j)
            T[m][j] -= coeff * T[i][j];
    }

    // Initial basis: artificial for each constraint row
    std::vector<int> basis(m);
    for (int i = 0; i < m; ++i) basis[i] = n + m + i;

    // ----- Primal simplex (minimisation) -----
    for (int iter = 0; iter < 200000; ++iter) {
        // Find entering variable: most negative reduced cost
        int    enter   = -1;
        double min_rc  = -1.0e-8;
        for (int j = 0; j < nvars; ++j) {
            if (T[m][j] < min_rc) { min_rc = T[m][j]; enter = j; }
        }
        if (enter < 0) break;   // optimal

        // Minimum-ratio test (leaving variable)
        int    leave    = -1;
        double min_ratio = 1.0e30;
        for (int i = 0; i < m; ++i) {
            if (T[i][enter] > 1.0e-10) {
                double ratio = T[i][RHS] / T[i][enter];
                if (ratio < min_ratio - 1.0e-12) { min_ratio = ratio; leave = i; }
            }
        }
        if (leave < 0) return false;   // unbounded

        // Pivot
        basis[leave] = enter;
        const double piv = T[leave][enter];
        for (int j = 0; j <= nvars; ++j) T[leave][j] /= piv;
        for (int i = 0; i <= m; ++i) {
            if (i == leave) continue;
            const double f = T[i][enter];
            if (std::abs(f) < 1.0e-15) continue;
            for (int j = 0; j <= nvars; ++j) T[i][j] -= f * T[leave][j];
        }
    }

    // Feasibility check: no artificial still basic with positive value
    for (int i = 0; i < m; ++i) {
        if (basis[i] >= n + m && T[i][RHS] > 1.0e-6) return false;
    }

    // Primal solution
    x.assign(n, 0.0);
    for (int i = 0; i < m; ++i)
        if (basis[i] < n) x[basis[i]] = T[i][RHS];

    // Dual variables: y_i = reduced cost of surplus s_i = T[m][n+i]
    // (By duality: c_{s_i}=0, column of s_i is -e_i, so T[m][n+i] = y_i >= 0)
    y.resize(m);
    for (int i = 0; i < m; ++i) y[i] = T[m][n + i];

    return true;
}

// ---------------------------------------------------------------------------
// Unbounded knapsack (pricing sub-problem for one stock type).
//
// Maximise  sum_i values[i] * a_i
// subject to sum_i weights[i] * a_i <= capacity,  a_i in {0,1,2,...}
//
// The weights are floating-point; we discretise to N_BINS integer bins.
// Returns (best_value, pattern_a[]).
// ---------------------------------------------------------------------------
std::pair<double, std::vector<int>> solveKnapsackDP(
    const std::vector<double>& values,
    const std::vector<double>& weights,
    double                     capacity,
    int                        m)
{
    std::vector<int> pattern(m, 0);
    if (capacity < 1.0e-12) return {0.0, pattern};

    const int    N_BINS   = 20000;
    const double bin_size = capacity / N_BINS;

    // Convert to integer weights (at least 1 bin per item)
    std::vector<int> w(m);
    for (int i = 0; i < m; ++i)
        w[i] = std::max(1, static_cast<int>(std::round(weights[i] / bin_size)));

    // DP table: dp[c] = best value achievable with c bins of capacity
    std::vector<double> dp(N_BINS + 1, 0.0);
    std::vector<int>    last(N_BINS + 1, -1);   // last item selected (for backtrack)

    for (int c = 1; c <= N_BINS; ++c) {
        for (int i = 0; i < m; ++i) {
            if (w[i] <= c && values[i] > 1.0e-12) {
                double v = dp[c - w[i]] + values[i];
                if (v > dp[c] + 1.0e-12) {
                    dp[c]   = v;
                    last[c] = i;
                }
            }
        }
    }

    // Backtrack to recover pattern
    int c = N_BINS;
    while (c > 0 && last[c] >= 0) {
        const int i = last[c];
        pattern[i]++;
        c -= w[i];
    }

    return {dp[N_BINS], pattern};
}

// ---------------------------------------------------------------------------
// Gilmore-Gomory column generation across ALL stock lengths simultaneously.
//
// Objective: minimise sum_j L_{k(j)} * x_j  (total bar-length consumed)
// This naturally prefers shorter bars over longer ones when both can cover
// the same demand, thereby reducing overall scrap.
//
// Pricing for stock type k:
//   max  sum_i y_i * a_i   s.t.  sum_i (d_i+c)*a_i <= L_k+c,  a_i >= 0 int
//   Add column if  max  > L_k   (reduced cost = L_k - sum y_i*a_i < 0)
//
// Outputs:
//   patterns – (stock_type_idx, counts[]) for each generated pattern
//   usage    – rounded-up integer usage of each pattern
// ---------------------------------------------------------------------------
void runColumnGenerationMultiType(
    const std::vector<double>& lengths,         // m distinct piece lengths
    const std::vector<int>&    demands,
    const std::vector<double>& stock_lengths,   // K available stock lengths
    double                     cut_w,
    std::vector<std::pair<int, std::vector<int>>>& patterns,
    std::vector<int>&                              usage)
{
    const int m = static_cast<int>(lengths.size());
    const int K = static_cast<int>(stock_lengths.size());
    patterns.clear();
    std::vector<double> costs;   // objective coefficient per pattern = L_k

    // ---- Initial patterns: for each stock type k and piece type i ----
    for (int k = 0; k < K; ++k) {
        for (int i = 0; i < m; ++i) {
            if (lengths[i] > stock_lengths[k] + 1.0e-9) continue;
            std::vector<int> p(m, 0);
            const double cap  = stock_lengths[k] + cut_w;
            const double item = lengths[i] + cut_w;
            p[i] = (item > 1.0e-12) ? std::max(1, static_cast<int>(cap / item)) : 1;
            patterns.push_back({k, p});
            costs.push_back(stock_lengths[k]);
        }
    }

    const std::vector<double> dbl_demand(demands.begin(), demands.end());

    // ---- Column-generation loop ----
    for (int iter = 0; iter < 300; ++iter) {
        const int np = static_cast<int>(patterns.size());

        std::vector<std::vector<double>> A(m, std::vector<double>(np));
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < np; ++j)
                A[i][j] = static_cast<double>(patterns[j].second[i]);

        std::vector<double> x_lp, y;
        if (!solveLPRelaxation(A, dbl_demand, costs, x_lp, y)) break;

        // For each stock type, price out a new column
        bool any_added = false;
        for (int k = 0; k < K; ++k) {
            // Pieces that don't fit in this stock type get zero value
            std::vector<double> vals(m), wts(m);
            for (int i = 0; i < m; ++i) {
                vals[i] = (lengths[i] <= stock_lengths[k] + 1.0e-9) ? y[i] : 0.0;
                wts[i]  = lengths[i] + cut_w;
            }
            const double cap = stock_lengths[k] + cut_w;

            auto [best_val, new_pat] = solveKnapsackDP(vals, wts, cap, m);

            // Reduced cost = L_k - best_val; add column when negative
            if (best_val <= stock_lengths[k] + 1.0e-6) continue;

            // Guard against discretisation error
            double used = -cut_w;
            for (int i = 0; i < m; ++i) used += new_pat[i] * (lengths[i] + cut_w);
            if (used > stock_lengths[k] + 1.0e-6) continue;

            // Skip duplicates (same stock type + same counts)
            bool dup = false;
            for (const auto& [pk, pp] : patterns)
                if (pk == k && pp == new_pat) { dup = true; break; }
            if (dup) continue;

            patterns.push_back({k, new_pat});
            costs.push_back(stock_lengths[k]);
            any_added = true;
        }
        if (!any_added) break;
    }

    // ---- Final LP → round up to integers ----
    {
        const int np = static_cast<int>(patterns.size());
        std::vector<std::vector<double>> A(m, std::vector<double>(np));
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < np; ++j)
                A[i][j] = static_cast<double>(patterns[j].second[i]);

        std::vector<double> x_lp, y;
        usage.assign(np, 0);
        if (solveLPRelaxation(A, dbl_demand, costs, x_lp, y)) {
            for (int j = 0; j < np; ++j)
                usage[j] = static_cast<int>(std::ceil(x_lp[j] - 1.0e-9));
        } else {
            usage.assign(np, 1);
        }
    }
}

} // anonymous namespace


// ============================================================================
//  CuttingStockOptimizer implementation
// ============================================================================

defineType(CuttingStockOptimizer);

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    CuttingStockOptimizer, &CuttingStockOptimizer::insertrule);

// ----------------------------------------------------------------------------
CuttingStockOptimizer::CuttingStockOptimizer(
    ScalarPtr               cutWidth,
    const StockList&        stock,
    const std::vector<FeaturePtr>& features)
    : cutWidth_(cutWidth), stock_(stock), features_(features)
{}

// ----------------------------------------------------------------------------
size_t CuttingStockOptimizer::calcHash() const
{
    ParameterListHash h;
    if (cutWidth_) h += *cutWidth_;
    for (const auto& f : features_) if (f) h += *f;
    for (const auto& s : stock_) if (s) h += *s;
    return h.getHash();
}

// ----------------------------------------------------------------------------
void CuttingStockOptimizer::build()
{
    pieces_.clear();
    usedStock_.clear();

    // ------------------------------------------------------------------ 1.
    // Extract all linear edges from every input feature.
    // ------------------------------------------------------------------
    for (int fi = 0; fi < static_cast<int>(features_.size()); ++fi) {
        const FeaturePtr& feat = features_[fi];
        if (!feat) continue;
        const std::string fname = feat->featureSymbolName();

        for (FeatureID eid : feat->allEdgesSet()) {
            if (feat->edgeType(eid) == GeomAbs_Line) {
                double len = edgeLength(feat->edge(eid));
                pieces_.push_back({fi, fname, static_cast<int>(eid), len, -1});
            }
        }
    }

    if (pieces_.empty()) {
        std::cout << "CuttingStockOptimizer: no linear edges found.\n";
        return;
    }

    // ------------------------------------------------------------------ 2.
    // Group pieces by distinct length (relative tolerance 1e-4).
    // ------------------------------------------------------------------
    double max_len = 0.0;
    for (const auto& p : pieces_) max_len = std::max(max_len, p.length);
    const double len_tol = 1.0e-4 * max_len;

    std::vector<double> distinct_lengths;
    std::vector<int>    total_demand;      // total demand per type (all stock combined)
    std::vector<int>    piece_to_type(pieces_.size(), -1);

    for (int pi = 0; pi < static_cast<int>(pieces_.size()); ++pi) {
        int found = -1;
        for (int t = 0; t < static_cast<int>(distinct_lengths.size()); ++t) {
            if (std::abs(pieces_[pi].length - distinct_lengths[t]) < len_tol) {
                found = t; break;
            }
        }
        if (found < 0) {
            distinct_lengths.push_back(pieces_[pi].length);
            total_demand.push_back(0);
            found = static_cast<int>(distinct_lengths.size()) - 1;
        }
        total_demand[found]++;
        piece_to_type[pi] = found;
    }

    const int m_types = static_cast<int>(distinct_lengths.size());

    // ------------------------------------------------------------------ 3.
    // Collect stock lengths (preserve original order for stockTypeIdx).
    // ------------------------------------------------------------------
    std::vector<double> stock_lengths;
    for (const auto& s : stock_) stock_lengths.push_back(s->value());

    const double cut_w = cutWidth_->value();

    // ------------------------------------------------------------------ 4.
    // Multi-type Gilmore-Gomory: one combined LP over all stock lengths.
    // Objective = sum L_k * x_{k,j}  → naturally prefers shorter bars.
    // ------------------------------------------------------------------
    std::vector<std::pair<int, std::vector<int>>> patterns;  // (stockTypeIdx, counts[])
    std::vector<int>                              usage;
    runColumnGenerationMultiType(
        distinct_lengths, total_demand, stock_lengths, cut_w, patterns, usage);

    // Build per-piece-type queues of unassigned piece indices
    std::vector<std::queue<int>> queues(m_types);
    for (int pi = 0; pi < static_cast<int>(pieces_.size()); ++pi)
        queues[piece_to_type[pi]].push(pi);

    // Assign pieces to stock items, one pattern application at a time
    for (int j = 0; j < static_cast<int>(patterns.size()); ++j) {
        const int    stk_type = patterns[j].first;
        const double stk_len  = stock_lengths[stk_type];
        const auto&  pat      = patterns[j].second;

        for (int rep = 0; rep < usage[j]; ++rep) {
            UsedStockItem item;
            item.stockTypeIdx = stk_type;
            item.stockLength  = stk_len;
            double pieces_len   = 0.0;
            int    n_pieces_cut = 0;

            for (int t = 0; t < m_types; ++t) {
                for (int cnt = 0; cnt < pat[t]; ++cnt) {
                    if (queues[t].empty()) continue;
                    const int pi = queues[t].front();
                    queues[t].pop();
                    item.pieceIndices.push_back(pi);
                    pieces_[pi].stockItemIdx = static_cast<int>(usedStock_.size());
                    pieces_len += distinct_lengths[t];
                    ++n_pieces_cut;
                }
            }
            if (n_pieces_cut == 0) continue;

            item.remainingLength =
                stk_len - pieces_len
                - static_cast<double>(std::max(0, n_pieces_cut - 1)) * cut_w;
            usedStock_.push_back(std::move(item));
        }
    }

    // ------------------------------------------------------------------ 5.
    // Post-processing downgrade pass.
    //
    // The LP relaxation can assign pieces to an over-large bar (because
    // fractional usage of a long bar appears cheaper than integer usage of a
    // short one, e.g. 0.5×3505 < 1×1800).  After integer rounding, switch
    // each stock item to the shortest available stock type that still fits
    // all its pieces.  Run this before the merge pass so the merge can then
    // consolidate same-type items.
    // ------------------------------------------------------------------
    {
        // Build indices sorted by stock length ascending
        std::vector<int> asc_k(stock_lengths.size());
        std::iota(asc_k.begin(), asc_k.end(), 0);
        std::sort(asc_k.begin(), asc_k.end(),
                  [&](int a, int b) { return stock_lengths[a] < stock_lengths[b]; });

        for (auto& item : usedStock_) {
            double total_len = 0.0;
            const int n = static_cast<int>(item.pieceIndices.size());
            for (int pi : item.pieceIndices) total_len += pieces_[pi].length;
            const double needed =
                total_len + static_cast<double>(std::max(0, n - 1)) * cut_w;

            for (int k : asc_k) {
                if (stock_lengths[k] >= needed - 1.0e-9) {
                    item.stockTypeIdx    = k;
                    item.stockLength     = stock_lengths[k];
                    item.remainingLength = stock_lengths[k] - needed;
                    break;
                }
            }
        }
    }

    // ------------------------------------------------------------------ 6.
    // Post-processing merge pass.
    //
    // Ceiling-rounding the LP relaxation can leave separately rounded
    // patterns that together under-fill a single bar (e.g. ceil(0.5)+ceil(0.4)=2
    // bars while the pieces actually fit in 1).  Greedily merge any pair of
    // same-type stock items whose combined pieces fit in one bar.
    // ------------------------------------------------------------------
    {
        bool merged = true;
        while (merged) {
            merged = false;
            for (int a = 0; a < static_cast<int>(usedStock_.size()) && !merged; ++a) {
                for (int b = a + 1; b < static_cast<int>(usedStock_.size()) && !merged; ++b) {
                    UsedStockItem& ia = usedStock_[a];
                    UsedStockItem& ib = usedStock_[b];
                    if (ia.stockTypeIdx != ib.stockTypeIdx) continue;

                    const int n_pieces =
                        static_cast<int>(ia.pieceIndices.size() + ib.pieceIndices.size());
                    double total_len = 0.0;
                    for (int pi : ia.pieceIndices) total_len += pieces_[pi].length;
                    for (int pi : ib.pieceIndices) total_len += pieces_[pi].length;
                    const double needed =
                        total_len + static_cast<double>(std::max(0, n_pieces - 1)) * cut_w;

                    if (needed <= ia.stockLength + 1.0e-9) {
                        for (int pi : ib.pieceIndices)
                            ia.pieceIndices.push_back(pi);
                        ia.remainingLength = ia.stockLength - needed;
                        usedStock_.erase(usedStock_.begin() + b);
                        merged = true;
                    }
                }
            }
        }
        // Rebuild stockItemIdx mapping after merges
        for (int si = 0; si < static_cast<int>(usedStock_.size()); ++si)
            for (int pi : usedStock_[si].pieceIndices)
                pieces_[pi].stockItemIdx = si;
    }

    // ------------------------------------------------------------------ 7.
    // Print results to std::cout.
    // ------------------------------------------------------------------
    using std::cout;
    using std::setw;
    using std::left;
    using std::right;
    using std::fixed;
    using std::setprecision;

    const int W_IDX  =  6;
    const int W_NAME = 24;
    const int W_EID  =  8;
    const int W_LEN  = 14;
    const int W_STK  = 12;

    auto hline = [&](char fill = '-') {
        cout << std::string(W_IDX + 1 + W_NAME + 1 + W_EID + 1 + W_LEN + 1 + W_STK, fill) << "\n";
    };

    // --- Table 1: one row per edge ---
    cout << "\n";
    cout << "=== CuttingStockOptimizer: Edge Assignment ===\n";
    hline('=');
    cout << right << setw(W_IDX)  << "Feat#"  << " "
         << left  << setw(W_NAME) << "Feature Name" << " "
         << right << setw(W_EID)  << "Edge ID" << " "
         << right << setw(W_LEN)  << "Length"  << " "
         << right << setw(W_STK)  << "StockItem#"
         << "\n";
    hline();
    for (const auto& p : pieces_) {
        cout << right << setw(W_IDX)  << p.featureIdx << " "
             << left  << setw(W_NAME) << p.featureName << " "
             << right << setw(W_EID)  << p.edgeId      << " "
             << right << setw(W_LEN)  << fixed << setprecision(4) << p.length << " "
             << right << setw(W_STK);
        if (p.stockItemIdx >= 0)
            cout << p.stockItemIdx;
        else
            cout << "UNASSIGNED";
        cout << "\n";
    }
    hline('=');

    // --- Table 2: one block per used stock item ---
    cout << "\n=== CuttingStockOptimizer: Stock Item Usage ===\n";

    for (int si = 0; si < static_cast<int>(usedStock_.size()); ++si) {
        const UsedStockItem& item = usedStock_[si];

        const int    orig_k   = item.stockTypeIdx;
        const double stk_len  = item.stockLength;

        cout << "\nStock item #" << si
             << "  (type " << orig_k
             << ", length " << fixed << setprecision(4) << stk_len << "):\n";
        hline('-');
        cout << right << setw(W_IDX)  << "Feat#"  << " "
             << left  << setw(W_NAME) << "Feature Name" << " "
             << right << setw(W_EID)  << "Edge ID" << " "
             << right << setw(W_LEN)  << "Length"
             << "\n";
        hline('-');

        for (int pi : item.pieceIndices) {
            const PieceInfo& p = pieces_[pi];
            cout << right << setw(W_IDX)  << p.featureIdx << " "
                 << left  << setw(W_NAME) << p.featureName << " "
                 << right << setw(W_EID)  << p.edgeId << " "
                 << right << setw(W_LEN)  << fixed << setprecision(4) << p.length
                 << "\n";
        }
        hline('-');
        cout << "  Remaining uncut length: "
             << fixed << setprecision(4) << item.remainingLength << "\n";
    }

    // Report unassigned pieces (insufficient stock)
    bool any_unassigned = false;
    for (const auto& p : pieces_) {
        if (p.stockItemIdx < 0) { any_unassigned = true; break; }
    }
    if (any_unassigned) {
        cout << "\n*** WARNING: the following edges could not be assigned (insufficient stock) ***\n";
        hline('*');
        cout << right << setw(W_IDX)  << "Feat#"  << " "
             << left  << setw(W_NAME) << "Feature Name" << " "
             << right << setw(W_EID)  << "Edge ID" << " "
             << right << setw(W_LEN)  << "Length"
             << "\n";
        hline('*');
        for (const auto& p : pieces_) {
            if (p.stockItemIdx >= 0) continue;
            cout << right << setw(W_IDX)  << p.featureIdx << " "
                 << left  << setw(W_NAME) << p.featureName << " "
                 << right << setw(W_EID)  << p.edgeId << " "
                 << right << setw(W_LEN)  << fixed << setprecision(4) << p.length
                 << "\n";
        }
        hline('*');
    }

    cout << std::flush;
}

// ----------------------------------------------------------------------------
void CuttingStockOptimizer::write(std::ostream&) const
{}

// ----------------------------------------------------------------------------
void CuttingStockOptimizer::insertrule(parser::ISCADParser& ruleset)
{
    // ISCAD syntax:
    //   CuttingStock( <cutWidth>, <length1>, <length2>, ... )
    //       << feat1, feat2, ... ;
    //
    // Synthesised attributes (in sequence, ignoring literals):
    //   _1 = ScalarPtr          (cutWidth)
    //   _2 = vector<ScalarPtr>  (available stock lengths)
    //   _3 = vector<FeaturePtr> (input features)

    ruleset.postProcFunctionRules.add(
        "CuttingStock",
        std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
            (
                qi::lit('(')
                > ruleset.r_scalarExpression                        // _1: cut width
                > qi::lit(',')
                > (ruleset.r_scalarExpression % qi::lit(','))       // _2: stock lengths
                > qi::lit(')')
                > qi::lit("<<")
                > (ruleset.r_solidmodel_expression % qi::lit(','))  // _3: features
                > qi::lit(';')
            )
            [
                qi::_val = phx::bind(
                    &CuttingStockOptimizer::create<
                        ScalarPtr,
                        const CuttingStockOptimizer::StockList&,
                        const std::vector<FeaturePtr>&>,
                    qi::_1, qi::_2, qi::_3)
            ]
        )
    );
}

} // namespace cad
} // namespace insight
