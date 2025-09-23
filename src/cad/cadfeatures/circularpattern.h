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

#ifndef INSIGHT_CAD_CIRCULARPATTERN_H
#define INSIGHT_CAD_CIRCULARPATTERN_H

#include "compound.h"

namespace insight {
namespace cad {

class CircularPattern
: public Compound
{

    FeaturePtr m1_;

    /**
     * pointer to another circular pattern feature, from which the parameters shall be copied
     */
    // FeaturePtr otherpat_;

    /**
     * alternative: parameters
     */
    struct ExplicitTransformation
    : public DependencySource
    {
        VectorPtr p0_;
        VectorPtr axis_;
        ScalarPtr n_;
        bool center_;

        void replaceDependency(const DependencyReplacement& repl) override;

        std::shared_ptr<DependencySource>
            shallowClone(TreeCloneMap& tcm) const override;
    };

    boost::variant<FeaturePtr,ExplicitTransformation> transformation_;

    std::string filterrule_;

    CircularPattern(const CircularPattern&o, TreeCloneMap& tcm);
    CircularPattern(FeaturePtr m1, VectorPtr p0, VectorPtr axis, ScalarPtr n, bool center=false, const std::string& filterrule="");
    CircularPattern(FeaturePtr m1, FeaturePtr otherpat);

    size_t calcHash() const override;
    void build() override;

public:
    declareType("CircularPattern");
#ifndef SWIG
    DEPENDS_W_BASE(Compound, (m1_));
#endif
    CREATE_FUNCTION(CircularPattern);
    CLONEABLE(CircularPattern);

    static void insertrule(parser::ISCADParser& ruleset);
    static FeatureCmdInfoList ruleDocumentation();
};


}
}

#endif // INSIGHT_CAD_CIRCULARPATTERN_H
