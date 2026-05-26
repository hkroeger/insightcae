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

#ifndef INSIGHT_CAD_SAILCUT3D_H
#define INSIGHT_CAD_SAILCUT3D_H

#include "cadfeature.h"
#include "cadfeatures/compound.h"


namespace insight {
namespace cad {


/**
 * @brief The SailCut3D class
 * Reads a SailCut *.panel3d XML file and creates a TopoDS_Compound
 * containing one TopoDS_Face per sail panel.
 *
 * The panel3d format stores a CPanelGroup with CPanel children.
 * Each CPanel has four CSide boundary curves (left, right, top, bottom),
 * each consisting of a sequence of CPoint3d vertices.
 */
class SailCut3D
    : public Compound
{
    boost::filesystem::path filepath_;

    SailCut3D(const SailCut3D& o, TreeCloneMap& tcm);
    explicit SailCut3D(const boost::filesystem::path& filepath);

    size_t calcHash() const override;
    void build() override;

public:
    declareType("SailCut3D");
    void replaceDependency(const DependencyReplacement& repl) override;
    void addDependencies(DependencyList& dl) const override;
    CREATE_FUNCTION(SailCut3D);
    CLONEABLE(SailCut3D);

    static void insertrule(parser::ISCADParser& ruleset);
    static FeatureCmdInfoList ruleDocumentation();
};


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SAILCUT3D_H
