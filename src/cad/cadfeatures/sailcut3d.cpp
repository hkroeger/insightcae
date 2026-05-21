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
#include <vector>

#include "BRep_Builder.hxx"
#include "BRepBuilderAPI_MakeEdge.hxx"
#include "BRepOffsetAPI_MakeFilling.hxx"
#include "GeomAPI_PointsToBSpline.hxx"
#include "TColgp_Array1OfPnt.hxx"
#include "TopoDS_Compound.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS.hxx"
#include "cadfeatures/importsolidmodel.h"
#include "gp_Pnt.hxx"
#include "gp_Vec.hxx"
#include "Precision.hxx"


#include "cadfeatures/fillingface.h"


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
            if (!boost::filesystem::exists(fp))
                throw insight::Exception(
                    _("File not found: %s"), filepath_.string().c_str());
        }

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

        Compound::build();

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
