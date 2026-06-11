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

#include "sailcut3d.h"
#include "cadfeature.h"
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/tools.h"
#include "base/translations.h"

#include <boost/spirit/include/qi.hpp>
#include "base/rapidxml.h"

#include <iostream>
#include <numeric>
#include <vector>

#include "BRep_Builder.hxx"
#include "BRep_Tool.hxx"
#include "BRepBuilderAPI_MakeEdge.hxx"
#include "BRepOffsetAPI_MakeFilling.hxx"
#include "GeomAPI_PointsToBSpline.hxx"
#include "TColgp_Array1OfPnt.hxx"
#include "TopExp.hxx"
#include "TopoDS_Compound.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS.hxx"
#include "cadfeatures/importsolidmodel.h"
#include "featureset.h"
#include "gp_Pnt.hxx"
#include "gp_Vec.hxx"
#include "Precision.hxx"


#include "cadfeatures/fillingface.h"
#include "cadfeatures/wire.h"


namespace qi  = boost::spirit::qi;
namespace phx = boost::phoenix;

using namespace std;

namespace insight {
namespace cad {


defineType(SailCut3D);
addToStaticFunctionTable(Feature, SailCut3D, insertrule);
addToStaticFunctionTable(Feature, SailCut3D, ruleDocumentation);


namespace {


// Read a <real name="..." value="..."/> child element and return its value.
// Uses insight::findNode (base/rapidxml.h): findNode(father, nameAttr, tagName).
double readReal(const rapidxml::xml_node<char>& parent, const char* name)
{
    const auto* n = insight::findNode(parent, name, "real");
    if (!n)
        throw insight::Exception("Missing <real name=\"%s\"> element", name);
    return insight::getMandatoryAttribute<double>(*n, "value");
}


using Points = std::vector<gp_Pnt>;


// Parse all CPoint3d children of a <CSide> node.
// The points are wrapped in a <vector name="point" size="N"> element.
// Consecutive duplicate points (within Precision::Confusion()) are removed,
// since they cause GeomAPI_PointsToBSpline and BRepOffsetAPI_MakeFilling to fail.
Points parseSide(const rapidxml::xml_node<char>& sideNode)
{
    Points pts;
    const auto* vecNode = insight::findNode(sideNode, "point", "vector");
    if (!vecNode)
        return pts;
    for (const auto* n = vecNode->first_node("CPoint3d");
         n;
         n = n->next_sibling("CPoint3d"))
    {
        gp_Pnt p(readReal(*n, "x"), readReal(*n, "y"), readReal(*n, "z"));
        if (pts.empty() || !pts.back().IsEqual(p, Precision::Confusion()))
            pts.push_back(p);
    }
    return pts;
}

// Cumulative normalised arc-length parameters: s[0]=0, s[n-1]=1.
std::vector<double> arcParams(const Points& pts)
{
    std::vector<double> s(pts.size(), 0.0);
    for (size_t i = 1; i < pts.size(); ++i)
        s[i] = s[i-1] + pts[i-1].Distance(pts[i]);
    const double total = s.back();
    if (total > Precision::Confusion())
        for (auto& v : s) v /= total;
    return s;
}

// Interpolate a point on a polyline at normalised arc-length parameter t.
gp_Pnt evalAt(const Points& pts, const std::vector<double>& s, double t)
{
    for (size_t i = 1; i < s.size(); ++i)
    {
        if (s[i] >= t - Precision::Confusion())
        {
            const double span = s[i] - s[i-1];
            if (span < Precision::Confusion())
                return pts[i];
            const double alpha = std::max(0.0, std::min(1.0, (t - s[i-1]) / span));
            return pts[i-1].Translated(gp_Vec(pts[i-1], pts[i]).Scaled(alpha));
        }
    }
    return pts.back();
}

// Extract the sub-polyline between normalised params t0 and t1,
// including interpolated endpoints and all original points in (t0, t1).
Points extractSegment(const Points& pts, const std::vector<double>& s,
                      double t0, double t1)
{
    Points seg;
    seg.push_back(evalAt(pts, s, t0));
    for (size_t i = 0; i < pts.size(); ++i)
        if (s[i] > t0 + Precision::Confusion() && s[i] < t1 - Precision::Confusion())
            seg.push_back(pts[i]);
    const gp_Pnt endPt = evalAt(pts, s, t1);
    if (!seg.back().IsEqual(endPt, Precision::Confusion()))
        seg.push_back(endPt);
    return seg;
}

// Return the normalised arc-length parameters of interior kink points
// where the turning angle exceeds maxAngle.
std::vector<double> findKinkParams(const Points& pts, const std::vector<double>& s,
                                   double maxAngle)
{
    std::vector<double> result;
    for (size_t i = 1; i + 1 < pts.size(); ++i)
    {
        const gp_Vec d1(pts[i-1], pts[i]);
        const gp_Vec d2(pts[i],   pts[i+1]);
        if (d1.Magnitude() > Precision::Confusion() &&
            d2.Magnitude() > Precision::Confusion() &&
            d1.Angle(d2) > maxAngle)
            result.push_back(s[i]);
    }
    return result;
}

// Split a side into sub-polylines at the given sorted normalised parameters.
std::vector<Points> splitSide(const Points& pts, const std::vector<double>& s,
                               const std::vector<double>& splitParams)
{
    std::vector<double> bounds = {0.0};
    bounds.insert(bounds.end(), splitParams.begin(), splitParams.end());
    bounds.push_back(1.0);

    std::vector<Points> segs;
    segs.reserve(bounds.size() - 1);
    for (size_t i = 1; i < bounds.size(); ++i)
        segs.push_back(extractSegment(pts, s, bounds[i-1], bounds[i]));
    return segs;
}

// Split two anti-parallel opposite sides (sideA: t=0..1, sideB: t=0..1 where
// sideB's t=0 corresponds to sideA's t=1) at the union of their kink parameters,
// cross-mapped via the 1-t symmetry.
// Returns the sorted interior split parameters for sideA (in sideA's t space).
// The corresponding splits for sideB are {1 - t} for each t in the returned vector.
std::vector<double> splitOppositePair(const Points& sideA, const Points& sideB,
                                      double maxAngle,
                                      const std::string& labelA = "A",
                                      const std::string& labelB = "B")
{
    const auto sA = arcParams(sideA);
    const auto sB = arcParams(sideB);

    auto kinksA = findKinkParams(sideA, sA, maxAngle);
    auto kinksB = findKinkParams(sideB, sB, maxAngle);

    // Cross-map: kink at t on sideA → split at (1-t) on sideB and vice versa.
    // Collect everything in sideA's coordinate system.
    std::vector<double> splitsA = kinksA;
    for (double t : kinksB) splitsA.push_back(1.0 - t);

    std::vector<double> splitsB = kinksB;
    for (double t : kinksA) splitsB.push_back(1.0 - t);

    auto cleanParams = [](std::vector<double>& v) {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end(),
                    [](double a, double b){ return b - a < 1e-6; }), v.end());
        v.erase(std::remove_if(v.begin(), v.end(),
                    [](double t){ return t < 1e-9 || t > 1.0 - 1e-9; }), v.end());
    };
    cleanParams(splitsA);
    cleanParams(splitsB);

    auto printParams = [](const std::string& label,
                          const std::vector<double>& kinks,
                          const std::vector<double>& splits)
    {
        std::cout << "  side " << label << ": "
                  << kinks.size() << " kink(s)";
        if (!kinks.empty())
        {
            std::cout << " at t =";
            for (double t : kinks) std::cout << " " << t;
        }
        std::cout << "  =>  " << splits.size() << " forced split(s)";
        if (!splits.empty())
        {
            std::cout << " at t =";
            for (double t : splits) std::cout << " " << t;
        }
        std::cout << "  (" << splits.size() + 1 << " segment(s))\n";
    };

    printParams(labelA, kinksA, splitsA);
    printParams(labelB, kinksB, splitsB);

    return splitsA; // caller derives sideB splits as {1 - t}
}


// Find the indices of "corner" points (direction kink > minAngle) in a closed polyline.
// The polyline is treated as closed: point [n-1] connects back to [0].
std::vector<size_t> findCornersOnClosedPolyline(const Points& pts, double minAngle)
{
    const size_t n = pts.size();
    std::vector<size_t> result;
    for (size_t i = 0; i < n; ++i)
    {
        gp_Vec d1(pts[(i + n - 1) % n], pts[i]);
        gp_Vec d2(pts[i],               pts[(i + 1) % n]);
        if (d1.Magnitude() > Precision::Confusion() &&
            d2.Magnitude() > Precision::Confusion() &&
            d1.Angle(d2) > minAngle)
            result.push_back(i);
    }
    return result;
}


// Build a smooth BSpline edge through the given ordered point list.
// For exactly 2 points a straight line segment is returned.
TopoDS_Edge makeSplineEdge(const std::vector<gp_Pnt>& pts)
{
    if (pts.size() < 2)
        throw insight::Exception("Need at least 2 points to create an edge");

    if (pts.size() == 2)
        return BRepBuilderAPI_MakeEdge(pts[0], pts[1]);

    TColgp_Array1OfPnt arr(1, static_cast<int>(pts.size()));
    for (int i = 0; i < static_cast<int>(pts.size()); ++i)
        arr.SetValue(i + 1, pts[i]);

    GeomAPI_PointsToBSpline builder(arr, 2, 2);
    if (!builder.IsDone())
        throw insight::Exception("GeomAPI_PointsToBSpline failed");

    BRepBuilderAPI_MakeEdge bld(builder.Curve());
    return bld.Edge();
}


} // anonymous namespace


SailCut3D::SailCut3D(const SailCut3D& o, TreeCloneMap& tcm)
    : Compound(o, tcm), filepath_(o.filepath_)
{}


SailCut3D::SailCut3D(const boost::filesystem::path& filepath)
    : filepath_(filepath)
{}


size_t SailCut3D::calcHash() const
{
    ParameterListHash h;
    h += this->type();
    h += filepath_;
    return h.getHash();
}


void SailCut3D::build()
{
    ExecTimer t("SailCut3D::build() [" + featureSymbolName() + "]");

    if (!cache.contains(hash()))
    {
        // Resolve file path (may be relative to shared model paths)
        boost::filesystem::path fp = filepath_;
        if (!boost::filesystem::exists(fp))
        {
            fp = sharedModelFilePath(filepath_.string());
        }

        if (!boost::filesystem::exists(fp))
            throw insight::Exception(
                _("File not found: %s"), filepath_.string().c_str());

        if (boost::filesystem::is_directory(fp))
            throw insight::Exception(
                _("File is a directory: %s"), filepath_.string().c_str());

        // insight::XMLDocument handles file I/O and rapidxml parsing.
        // The panel3d root element is <CPanelGroup>, not <root>, so
        // doc.rootNode is null; we call first_node() on the document directly.
        insight::XMLDocument doc(fp, "CSailDoc");
        const auto* root = doc.rootNode->first_node("CPanelGroup");
        if (!root)
            throw insight::Exception(
                "No <CPanelGroup> root element found in %s",
                fp.string().c_str());


        // CPanel elements are wrapped in <vector name="panel" size="N">
        const auto* panelVec = insight::findNode(*root, "panel", "vector");
        if (!panelVec)
            throw insight::Exception(
                "No <vector name=\"panel\"> found in CPanelGroup in %s",
                fp.string().c_str());

        int panelIndex = 0;
        int addedFaces = 0;

        // All panel sides collected for topological boundary detection.
        // A side is on the sail boundary iff no other panel has a side with
        // the same two endpoints — interior seams are always shared between
        // exactly two panels regardless of cut type (horizontal, vertical,
        // radial, mitre, …).
        std::vector<Points> allPanelSides;

        for (const auto* pNode = panelVec->first_node("CPanel");
             pNode;
             pNode = pNode->next_sibling("CPanel"), ++panelIndex)
        {
            try
            {
                auto name=insight::getMandatoryAttribute(*pNode, "name");
                std::cout<<"processing panel "<<name<<std::endl;

                // insight::findNode(father, nameAttrValue, tagName)
                const auto* leftNode   = insight::findNode(*pNode, "left",   "CSide");
                const auto* rightNode  = insight::findNode(*pNode, "right",  "CSide");
                const auto* topNode    = insight::findNode(*pNode, "top",    "CSide");
                const auto* bottomNode = insight::findNode(*pNode, "bottom", "CSide");

                if (!leftNode || !rightNode || !topNode || !bottomNode)
                    throw insight::Exception(
                        "Panel %d: one or more side elements missing", panelIndex);

                const auto left   = parseSide(*leftNode);
                const auto right  = parseSide(*rightNode);
                const auto top    = parseSide(*topNode);
                const auto bottom = parseSide(*bottomNode);

                // Collect all four sides for post-loop boundary detection.
                for (const Points* s : {&left, &right, &top, &bottom})
                    if (s->size() >= 2)
                        allPanelSides.push_back(*s);

                // if (left.size() < 2 || right.size() < 2 ||
                //     top.size()  < 2 || bottom.size() < 2)
                //     throw insight::Exception(
                //         "Panel %d: a side has fewer than 2 points", panelIndex);

                // SailCut panel orientation:
                //   left   : luff,  from foot (index 0) to head (index n-1)
                //   right  : leech, from foot (index 0) to head (index n-1)
                //   bottom : foot,  from luff (index 0) to leech (index m-1)
                //   top    : head,  from luff (index 0) to leech (index m-1)
                //
                // Closed boundary traversal (counterclockwise):
                //   bottom:       bottom[0]   -> bottom.back()
                //   right:        right[0]    -> right.back()
                //   top reversed: top.back()  -> top[0]
                //   left reversed:left.back() -> left[0]

                const Points topRev(top.rbegin(),  top.rend());
                const Points leftRev(left.rbegin(), left.rend());

                // Determine split parameters along each pair of opposite sides.
                // splitsH are interior split t-values in bottom's parameterisation (luff→leech).
                // splitsV are interior split t-values in right's parameterisation (foot→head).
                constexpr double kinkAngle = M_PI / 6.0; // 30 degrees
                const auto splitsH = splitOppositePair(
                    bottom, topRev,  kinkAngle, "bottom", "top");
                const auto splitsV = splitOppositePair(
                    right,  leftRev, kinkAngle, "right",  "left");

                // Build boundary vectors: [0, split..., 1]
                std::vector<double> boundsH = {0.0};
                boundsH.insert(boundsH.end(), splitsH.begin(), splitsH.end());
                boundsH.push_back(1.0);

                std::vector<double> boundsV = {0.0};
                boundsV.insert(boundsV.end(), splitsV.begin(), splitsV.end());
                boundsV.push_back(1.0);

                const int NH = static_cast<int>(boundsH.size()) - 1;
                const int NV = static_cast<int>(boundsV.size()) - 1;

                std::cout << "  sub-panel grid: " << NH << " x " << NV << "\n";

                // Arc-length params for all 4 sides in their natural orientation.
                const auto sBottom  = arcParams(bottom);
                const auto sTop     = arcParams(top);
                const auto sRight   = arcParams(right);
                const auto sLeft    = arcParams(left);
                const auto sTopRev  = arcParams(topRev);
                const auto sLeftRev = arcParams(leftRev);

                // Coons bilinear interpolation: P(u,v) where
                //   u ∈ [0,1] = luff→leech, v ∈ [0,1] = foot→head.
                const gp_Pnt P00 = evalAt(bottom, sBottom, 0.0); // luff-foot
                const gp_Pnt P10 = evalAt(bottom, sBottom, 1.0); // leech-foot
                const gp_Pnt P01 = evalAt(top,    sTop,    0.0); // luff-head
                const gp_Pnt P11 = evalAt(top,    sTop,    1.0); // leech-head

                auto coons = [&](double u, double v) -> gp_Pnt
                {
                    const gp_Pnt Rb = evalAt(bottom, sBottom, u);
                    const gp_Pnt Rt = evalAt(top,    sTop,    u);
                    const gp_Pnt Cl = evalAt(left,   sLeft,   v);
                    const gp_Pnt Cr = evalAt(right,  sRight,  v);
                    const double x =
                        (1-v)*Rb.X() + v*Rt.X() + (1-u)*Cl.X() + u*Cr.X()
                        - ((1-u)*(1-v)*P00.X() + u*(1-v)*P10.X()
                         + (1-u)*v   *P01.X() + u*v    *P11.X());
                    const double y =
                        (1-v)*Rb.Y() + v*Rt.Y() + (1-u)*Cl.Y() + u*Cr.Y()
                        - ((1-u)*(1-v)*P00.Y() + u*(1-v)*P10.Y()
                         + (1-u)*v   *P01.Y() + u*v    *P11.Y());
                    const double z =
                        (1-v)*Rb.Z() + v*Rt.Z() + (1-u)*Cl.Z() + u*Cr.Z()
                        - ((1-u)*(1-v)*P00.Z() + u*(1-v)*P10.Z()
                         + (1-u)*v   *P01.Z() + u*v    *P11.Z());
                    return {x, y, z};
                };

                // Pre-compute grid corner points (col = 0..NH, row = 0..NV).
                std::vector<std::vector<gp_Pnt>> grid(
                    NH+1, std::vector<gp_Pnt>(NV+1));
                for (int col = 0; col <= NH; ++col)
                    for (int row = 0; row <= NV; ++row)
                        grid[col][row] = coons(boundsH[col], boundsV[row]);

                // Interior edge generators: isoparametric curves on the Coons surface.
                // makeHorizEdge: constant v = vFixed, u from u0 to u1.
                // makeVertEdge:  constant u = uFixed, v from v0 to v1.
                constexpr int N_INTERP = 10;
                auto makeHorizEdge = [&](double u0, double u1, double vFixed) -> Points
                {
                    Points pts;
                    pts.reserve(N_INTERP + 1);
                    for (int k = 0; k <= N_INTERP; ++k)
                        pts.push_back(coons(u0 + (u1 - u0) * k / N_INTERP, vFixed));
                    return pts;
                };
                auto makeVertEdge = [&](double uFixed, double v0, double v1) -> Points
                {
                    Points pts;
                    pts.reserve(N_INTERP + 1);
                    for (int k = 0; k <= N_INTERP; ++k)
                        pts.push_back(coons(uFixed, v0 + (v1 - v0) * k / N_INTERP));
                    return pts;
                };

                // Create one BRepOffsetAPI_MakeFilling per sub-panel (i, j).
                for (int i = 0; i < NH; ++i)
                {
                    for (int j = 0; j < NV; ++j)
                    {
                        // Four boundary polylines for this sub-panel:
                        // south: grid[i][j]    → grid[i+1][j]   (foot side, luff→leech)
                        // east:  grid[i+1][j]  → grid[i+1][j+1] (leech side, foot→head)
                        // north: grid[i+1][j+1]→ grid[i][j+1]   (head side, leech→luff)
                        // west:  grid[i][j+1]  → grid[i][j]     (luff side, head→foot)
                        //
                        // Boundary sub-panels use the original sampled curves.
                        // Interior edges use Coons isoparametric interpolation so
                        // they follow the surface curvature rather than a straight line.
                        Points south = (j == 0)
                            ? extractSegment(bottom, sBottom,
                                             boundsH[i], boundsH[i+1])
                            : makeHorizEdge(boundsH[i], boundsH[i+1], boundsV[j]);

                        Points east = (i == NH-1)
                            ? extractSegment(right, sRight,
                                             boundsV[j], boundsV[j+1])
                            : makeVertEdge(boundsH[i+1], boundsV[j], boundsV[j+1]);

                        // topRev[t] = top[1-t], so column i in bottom coords
                        // maps to topRev interval [1-boundsH[i+1], 1-boundsH[i]].
                        // Interior north edges go leech→luff (u decreasing).
                        Points north = (j == NV-1)
                            ? extractSegment(topRev, sTopRev,
                                             1.0-boundsH[i+1], 1.0-boundsH[i])
                            : makeHorizEdge(boundsH[i+1], boundsH[i], boundsV[j+1]);

                        // leftRev[t] = left[1-t], so row j in right coords
                        // maps to leftRev interval [1-boundsV[j+1], 1-boundsV[j]].
                        // Interior west edges go head→foot (v decreasing).
                        Points west = (i == 0)
                            ? extractSegment(leftRev, sLeftRev,
                                             1.0-boundsV[j+1], 1.0-boundsV[j])
                            : makeVertEdge(boundsH[i], boundsV[j+1], boundsV[j]);

                        BRepOffsetAPI_MakeFilling filling;
                        filling.SetApproxParam(3, 8);
                        int ns = 0;
                        for (const auto* edgePts : {&south, &east, &north, &west})
                        {
                            try
                            {
                                filling.Add(makeSplineEdge(*edgePts), GeomAbs_C0);
                                ++ns;
                            }
                            catch (...)
                            {
                                std::cout << "  skipping degenerate edge in sub-panel "
                                          << i << "," << j << "\n";
                            }
                        }

                        if (ns < 1)
                        {
                            std::cerr << "  no edges for sub-panel "
                                      << i << "," << j << " — skipping\n";
                            continue;
                        }

                        filling.Build();
                        if (!filling.IsDone())
                        {
                            std::cerr << "  filling failed for sub-panel "
                                      << i << "," << j << " of panel " << name << "\n";
                            continue;
                        }

                        const std::string subName =
                            name + "_" + std::to_string(i) + "_" + std::to_string(j);
                        components_[subName] = cad::Import::create(filling.Shape());
                        ++addedFaces;
                    }
                }
            }
            catch (const std::exception& ex)
            {
                std::cerr << "SailCut3D: skipping panel " << panelIndex
                          << ": " << ex.what() << "\n";
            }
        }

        if (addedFaces == 0)
            throw insight::Exception(
                "No valid panel faces could be built from %s",
                fp.string().c_str());

        // ── Sail corner reference points via topological boundary detection ──
        //
        // Step 1: mark interior seams.
        // Two sides are a shared seam when their two endpoints coincide
        // (in either order).  Boundary sides are those not matched.
        constexpr double seamTol = 1e-6;
        const size_t nSides = allPanelSides.size();
        std::vector<bool> isBnd(nSides, true);
        for (size_t a = 0; a < nSides; ++a)
        {
            if (!isBnd[a]) continue;
            const gp_Pnt fa = allPanelSides[a].front();
            const gp_Pnt la = allPanelSides[a].back();
            for (size_t b = a + 1; b < nSides; ++b)
            {
                if (!isBnd[b]) continue;
                const gp_Pnt fb = allPanelSides[b].front();
                const gp_Pnt lb = allPanelSides[b].back();
                if ((fa.IsEqual(fb, seamTol) && la.IsEqual(lb, seamTol)) ||
                    (fa.IsEqual(lb, seamTol) && la.IsEqual(fb, seamTol)))
                {
                    isBnd[a] = isBnd[b] = false;
                    break;
                }
            }
        }

        // Step 2: collect boundary polylines and assemble into one closed perimeter.
        std::vector<Points> bndSegs;
        for (size_t i = 0; i < nSides; ++i)
            if (isBnd[i])
                bndSegs.push_back(allPanelSides[i]);

        Points perimeter;
        if (!bndSegs.empty())
        {
            std::vector<Points> rem = std::move(bndSegs);
            perimeter = rem[0];
            rem.erase(rem.begin());
            while (!rem.empty())
            {
                const gp_Pnt& tip = perimeter.back();
                bool found = false;
                for (size_t i = 0; i < rem.size(); ++i)
                {
                    if (tip.IsEqual(rem[i].front(), seamTol))
                    {
                        for (size_t k = 1; k < rem[i].size(); ++k)
                            perimeter.push_back(rem[i][k]);
                        rem.erase(rem.begin() + i);
                        found = true; break;
                    }
                    if (tip.IsEqual(rem[i].back(), seamTol))
                    {
                        for (size_t k = rem[i].size() - 1; k-- > 0; )
                            perimeter.push_back(rem[i][k]);
                        rem.erase(rem.begin() + i);
                        found = true; break;
                    }
                }
                if (!found) break; // chain gap — partial sail?
            }
            // Drop closing duplicate if chain is fully closed.
            if (perimeter.size() > 1 &&
                perimeter.back().IsEqual(perimeter.front(), seamTol))
                perimeter.pop_back();
        }

        // Step 3: find sail corner points on the closed perimeter.
        // Use a 15° threshold — low enough to catch gentle gaff angles at the
        // throat, well above the < 5° kinks at smooth panel-to-panel junctions.
        constexpr double sailCornerAngle = M_PI / 12.0; // 15°
        const auto cornerIdxs =
            findCornersOnClosedPolyline(perimeter, sailCornerAngle);
        const size_t nCorners = cornerIdxs.size();
        std::cout << "SailCut3D: detected " << nCorners
                  << " sail corners on perimeter of "
                  << perimeter.size() << " points\n";

        // sideRanges: sail-side name → perimeter index range (from, to), inclusive.
        // Populated inside the nCorners block; used after Compound::build().
        std::map<std::string, std::pair<size_t,size_t>> sideRanges;

        if (nCorners >= 3 && nCorners <= 4)
        {
            // Collect corner points.
            std::vector<gp_Pnt> cp;
            for (size_t ci : cornerIdxs)
                cp.push_back(perimeter[ci]);

            // Step 4: assign tack / clew / throat / peak purely by coordinate.
            //
            // In SailCut's coordinate system Y is the vertical (height) axis and
            // X runs roughly from luff (mast / forestay, lower X) to leech (higher X).
            //
            // - The 2 corners with the lowest Y are the foot corners.
            //   Among those, lower X → tack, higher X → clew.
            // - The remaining corner(s) are the head.
            //   For a 4-corner sail: lower X → throat, higher X → peak.
            //   For a triangular sail: single head corner → throat = peak.

            std::vector<int> byY(nCorners);
            std::iota(byY.begin(), byY.end(), 0);
            std::sort(byY.begin(), byY.end(),
                [&](int a, int b){ return cp[a].Y() < cp[b].Y(); });

            // Foot pair: byY[0] and byY[1] (lowest Y = foot of sail)
            int fi0 = byY[0], fi1 = byY[1];
            int ci_tack = (cp[fi0].X() <= cp[fi1].X()) ? fi0 : fi1;
            int ci_clew = (cp[fi0].X() <= cp[fi1].X()) ? fi1 : fi0;
            gp_Pnt tack = cp[ci_tack];
            gp_Pnt clew = cp[ci_clew];

            int ci_throat, ci_peak;
            gp_Pnt throat, peak;
            if (nCorners == 4)
            {
                int hi0 = byY[2], hi1 = byY[3];
                ci_throat = (cp[hi0].X() <= cp[hi1].X()) ? hi0 : hi1;
                ci_peak   = (cp[hi0].X() <= cp[hi1].X()) ? hi1 : hi0;
                throat = cp[ci_throat];
                peak   = cp[ci_peak];
            }
            else // triangular sail: single head corner
            {
                ci_throat = ci_peak = byY[2];
                throat = peak = cp[ci_throat];
            }

            refpoints_["tack"]   = insight::vec3(tack);
            refpoints_["clew"]   = insight::vec3(clew);
            refpoints_["throat"] = insight::vec3(throat);
            refpoints_["peak"]   = insight::vec3(peak);

            std::cout << "  tack   " << tack.X()   << " " << tack.Y()   << " " << tack.Z()   << "\n"
                      << "  clew   " << clew.X()   << " " << clew.Y()   << " " << clew.Z()   << "\n"
                      << "  throat " << throat.X() << " " << throat.Y() << " " << throat.Z() << "\n"
                      << "  peak   " << peak.X()   << " " << peak.Y()   << " " << peak.Z()   << "\n";

            // Compute perimeter index ranges for each sail side.
            // cornerIdxs is in perimeter order; consecutive pairs bound a side.
            auto cornerName = [&](int ci) -> std::string {
                if (ci == ci_tack)   return "tack";
                if (ci == ci_clew)   return "clew";
                if (ci == ci_throat) return "throat";
                if (ci == ci_peak)   return "peak";
                return "";
            };
            auto sideNameForPair = [](const std::string& a, const std::string& b) -> std::string {
                auto both = [&](const std::string& x, const std::string& y){
                    return (a==x && b==y) || (a==y && b==x);
                };
                if (both("tack",   "clew"))    return "foot";
                if (both("tack",   "throat"))  return "luff";
                if (both("clew",   "peak"))    return "leech";
                if (both("throat", "peak"))    return "head";
                // Triangular sail: throat==peak; clew→throat is leech.
                if (both("clew",   "throat"))  return "leech";
                return "";
            };
            for (int k = 0; k < (int)nCorners; ++k)
            {
                int nextK = (k + 1) % (int)nCorners;
                std::string name = sideNameForPair(
                    cornerName(k), cornerName(nextK));
                if (!name.empty())
                    sideRanges[name] = { cornerIdxs[k], cornerIdxs[nextK] };
            }
        }
        else
        {
            std::cerr << "SailCut3D: unexpected corner count (" << nCorners
                      << ") — sail corner reference points not set\n";
        }

        Compound::build();

        // ── Sail-side edge feature sets ──────────────────────────────────────
        // After Compound::build() the topology index (idx_) is ready.
        // We access it directly (bypassing checkForBuildDuringAccess()) to
        // avoid a recursive-lock deadlock while still inside build().
        if (!sideRanges.empty() && idx_)
        {
            const size_t N = perimeter.size();

            // Returns true when perimeter index idx lies in [from, to]
            // following the forward (increasing) perimeter direction.
            auto inRange = [&](size_t idx, size_t from, size_t to) -> bool {
                if (from == to)  return idx == from;
                if (from < to)   return idx >= from && idx <= to;
                else             return idx >= from || idx <= to; // wraps
            };

            // Find the perimeter index for point p.
            // First try exact vertex match; if that fails, project p onto each
            // perimeter segment (handles interpolated split-point endpoints that
            // land between two perimeter vertices).  Returns the segment-start
            // index so the result is always inside the side's [from,to] range.
            constexpr double matchTol = 1e-3;
            auto perimIdx = [&](const gp_Pnt& p) -> int {
                for (size_t i = 0; i < N; ++i)
                    if (perimeter[i].IsEqual(p, matchTol))
                        return (int)i;
                // Segment-interior fallback for interpolated kink-split endpoints
                for (size_t i = 0; i < N; ++i) {
                    const size_t next = (i + 1) % N;
                    const gp_Vec seg(perimeter[i], perimeter[next]);
                    const double len2 = seg.SquareMagnitude();
                    if (len2 < Precision::Confusion()) continue;
                    const gp_Vec toP(perimeter[i], p);
                    const double t = std::max(0.0, std::min(1.0,
                        toP.Dot(seg) / len2));
                    const gp_Pnt proj = perimeter[i].Translated(seg.Scaled(t));
                    if (proj.Distance(p) < matchTol)
                        return (int)i; // classify at segment-start index
                }
                return -1;
            };

            std::map<std::string, std::vector<FeatureID>> sideEdgeIds;

            FeatureSetData allEdgeTags;
            idx_->insertAllEdgeTags(allEdgeTags);

            for (FeatureID eid : allEdgeTags)
            {
                const TopoDS_Edge& e = idx_->edgeByTag(eid);
                gp_Pnt p1 = BRep_Tool::Pnt(TopExp::FirstVertex(e));
                gp_Pnt p2 = BRep_Tool::Pnt(TopExp::LastVertex(e));
                int i1 = perimIdx(p1);
                int i2 = perimIdx(p2);
                if (i1 < 0 || i2 < 0) continue; // interior / seam edge

                for (const auto& [name, range] : sideRanges)
                {
                    if (inRange(i1, range.first, range.second) &&
                        inRange(i2, range.first, range.second))
                    {
                        sideEdgeIds[name].push_back(eid);
                        break;
                    }
                }
            }

            for (auto& [name, ids] : sideEdgeIds)
            {
                if (!ids.empty())
                {
                    auto efs=std::make_shared<FeatureSet>(
                        shared_from_this(), Edge, ids);

                    providedFeatureSets_[name] = efs;
                    std::cout << "SailCut3D: sail side \"" << name
                              << "\" — " << ids.size() << " edges\n";
                    providedSubshapes_[name]=cad::Wire::create(efs);
                }
            }
        }

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<SailCut3D>(hash()));
    }
}

void SailCut3D::replaceDependency(const DependencyReplacement &repl)
{
    invalidate();
}

void SailCut3D::addDependencies(DependencyList& dl) const
{}

void SailCut3D::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.modelstepFunctionRules.add(
        "SailCut3D",
        std::make_shared<parser::ISCADParser::ModelstepRule>(
            ('(' > ruleset.r_path > ')')
            [qi::_val = phx::bind(
                &SailCut3D::create<const boost::filesystem::path&>,
                qi::_1)]
        )
    );
}


FeatureCmdInfoList SailCut3D::ruleDocumentation()
{
    return {
        FeatureCmdInfo(
            "SailCut3D",
            "( <path> )",
            _("Imports a SailCut *.panel3d XML file and returns an OCC compound "
              "containing one face per sail panel. Each panel face is constructed "
              "from the four boundary curves (left/luff, right/leech, top/head, "
              "bottom/foot) stored in the file.")
        )
    };
}


} // namespace cad
} // namespace insight
