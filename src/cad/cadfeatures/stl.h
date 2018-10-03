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

#ifndef STL_H
#define STL_H

#include "cadfeature.h"
#include "MeshVS_Mesh.hxx"

namespace insight {
namespace cad {


class STL
    : public Feature
{
    boost::filesystem::path fname_;

    std::shared_ptr<gp_Trsf> trsf_;
    //or
    FeaturePtr other_trsf_;

    STL(const boost::filesystem::path& fname);
    STL(const boost::filesystem::path& fname, const gp_Trsf& trsf);
    STL(const boost::filesystem::path& fname, FeaturePtr other_trsf);


protected:
    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType("STL");
    STL();

    static FeaturePtr create
    (
        const boost::filesystem::path& fname
    );
    static FeaturePtr create_trsf
    (
        const boost::filesystem::path& fname,
        gp_Trsf trsf
    );
    static FeaturePtr create_other
    (
        const boost::filesystem::path& fname,
        FeaturePtr other_trsf
    );

//    virtual Handle_AIS_InteractiveObject buildVisualization() const;

    virtual void insertrule(parser::ISCADParser& ruleset) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;

};


}
}

#endif // STL_H
