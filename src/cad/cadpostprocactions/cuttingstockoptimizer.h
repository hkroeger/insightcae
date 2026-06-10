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

#ifndef INSIGHT_CAD_CUTTINGSTOCKOPTIMIZER_H
#define INSIGHT_CAD_CUTTINGSTOCKOPTIMIZER_H

#include "cadtypes.h"
#include "cadpostprocaction.h"

namespace insight {
namespace cad {

/**
 * @brief CuttingStockOptimizer
 *
 * Applies the Gilmore-Gomory column-generation method to optimally assign
 * all linear edges from a list of CAD features to stock bar items of given
 * lengths, minimising scrap.  The required number of bars for each available
 * length is computed by the algorithm.
 *
 * ISCAD syntax:
 * @code
 *   CuttingStock( <cutWidth>, <length1>, <length2>, ... )
 *       << feature1, feature2, ... ;
 * @endcode
 *
 * Two tables are written to std::cout:
 *  1. One row per input edge: feature index, feature name, edge id,
 *     edge length, assigned stock-item index.
 *  2. One block per used stock item: pieces cut from it (feature index,
 *     feature name, edge id, length) and the remaining uncut length.
 */
class CuttingStockOptimizer : public PostprocAction
{
public:
    /// Parser-facing type: list of available stock lengths
    typedef std::vector<ScalarPtr> StockList;

private:
    std::vector<FeaturePtr> features_;
    ScalarPtr               cutWidth_;
    StockList               stock_;

    // ---- results filled by build() ----------------------------------------
    struct PieceInfo {
        int         featureIdx;
        std::string featureName;
        int         edgeId;
        double      length;
        int         stockItemIdx; ///< index into usedStock_ (-1 = unassigned)
    };

    struct UsedStockItem {
        int               stockTypeIdx; ///< index in original stock_ list
        double            stockLength;
        std::vector<int>  pieceIndices; ///< indices into pieces_
        double            remainingLength;
    };

    std::vector<PieceInfo>    pieces_;
    std::vector<UsedStockItem> usedStock_;

    // -----------------------------------------------------------------------
    size_t calcHash() const override;
    void   build()    override;

    CuttingStockOptimizer(ScalarPtr               cutWidth,
                          const StockList&         stock,
                          const std::vector<FeaturePtr>& features);

public:
    declareType("CuttingStockOptimizer");
    CREATE_FUNCTION(CuttingStockOptimizer);

    void write(std::ostream&) const override;

    static void insertrule(parser::ISCADParser& ruleset);
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CUTTINGSTOCKOPTIMIZER_H
