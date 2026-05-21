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


// Parse all CPoint3d children of a <CSide> node.
// The points are wrapped in a <vector name="point" size="N"> element.
// Consecutive duplicate points (within Precision::Confusion()) are removed,
// since they cause GeomAPI_PointsToBSpline and BRepOffsetAPI_MakeFilling to fail.
std::vector<gp_Pnt> parseSide(const rapidxml::xml_node<char>& sideNode)
{
    std::vector<gp_Pnt> pts;
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


// Split a point sequence at interior kink points where the turning angle between
// consecutive segments exceeds maxAngle (radians). Each returned sub-sequence
// shares its first point with the last point of the previous one so the segments
// remain geometrically connected.
std::vector<std::vector<gp_Pnt>> splitAtKinks(
    const std::vector<gp_Pnt>& pts,
    double maxAngle = M_PI / 8.0)   // 30 degrees
{
    std::vector<std::vector<gp_Pnt>> segs;
    if (pts.size() < 2)
        return segs;

    std::vector<gp_Pnt> cur;
    cur.push_back(pts[0]);

    for (size_t i = 1; i < pts.size(); ++i)
    {
        cur.push_back(pts[i]);

        if (i + 1 < pts.size())
        {
            const gp_Vec d1(pts[i - 1], pts[i]);
            const gp_Vec d2(pts[i],     pts[i + 1]);
            if (d1.Magnitude() > Precision::Confusion() &&
                d2.Magnitude() > Precision::Confusion() &&
                d1.Angle(d2) > maxAngle)
            {
                segs.push_back(std::move(cur));
                cur.clear();
                cur.push_back(pts[i]); // shared kink point starts the next segment
            }
        }
    }

    segs.push_back(std::move(cur));
    return segs;
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

                const std::vector<gp_Pnt> topRev(top.rbegin(),  top.rend());
                const std::vector<gp_Pnt> leftRev(left.rbegin(), left.rend());

                BRepOffsetAPI_MakeFilling filling;
                filling.SetApproxParam(2,10);

                int ns=0;
                for (const auto& side: {bottom, right, topRev, leftRev})
                {
                    for (const auto& seg: splitAtKinks(side))
                    {
                        try
                        {
                            filling.Add(makeSplineEdge(seg), GeomAbs_C0);
                            ++ns;
                        }
                        catch (...)
                        {
                            std::cout << "skipping edge segment with too few points\n";
                        }
                    }
                }

                if (ns<1)
                    throw insight::Exception("Panel %d: no side left", panelIndex);

                filling.Build();

                if (!filling.IsDone())
                    throw insight::Exception(
                        "Surface filling failed for panel %d", panelIndex);

                components_[name] = cad::Import::create(filling.Shape());
                ++addedFaces;
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
