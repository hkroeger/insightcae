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

#ifndef INSIGHT_CAD_REFPLACE_H
#define INSIGHT_CAD_REFPLACE_H

#include "derivedfeature.h"

namespace insight {
namespace cad {

    
    

class Condition : public DependencySource
{
public:
    double residual ( const arma::mat& values ) const;
    virtual double residual ( const gp_Trsf& tr ) const =0;
};




class CoincidentPoint
    : public Condition
{
    VectorPtr p_org_,  p_targ_;

    CoincidentPoint(const CoincidentPoint&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((p_org_,  p_targ_));
#endif
    CLONEABLE(CoincidentPoint);

    CoincidentPoint ( VectorPtr p_org, VectorPtr p_targ );
    double residual ( const gp_Trsf& tr ) const override;
};




class ParallelAxis
    : public Condition
{
    VectorPtr dir_org_,  dir_targ_;
    ParallelAxis(const ParallelAxis&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((dir_org_,  dir_targ_));
#endif
    CLONEABLE(ParallelAxis);
    ParallelAxis ( VectorPtr dir_org, VectorPtr dir_targ );
    double residual ( const gp_Trsf& tr ) const override;
};




class ParallelPlanes
    : public Condition
{
public:
  enum Orientation { Same, Inverted, Undefined };

protected:
    DatumPtr pl_org_,  pl_targ_;
    Orientation orient_;
    ParallelPlanes(const ParallelPlanes&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((pl_org_,  pl_targ_));
#endif
    CLONEABLE(ParallelPlanes);
    ParallelPlanes ( DatumPtr pl_org, DatumPtr pl_targ, Orientation orient=Same );
    double residual ( const gp_Trsf& tr ) const override;
};




class AlignedPlanes
    : public ParallelPlanes
{
    AlignedPlanes(const AlignedPlanes&o, TreeCloneMap& tcm);
public:
    CLONEABLE(AlignedPlanes);
    AlignedPlanes ( DatumPtr pl_org, DatumPtr pl_targ, Orientation orient=Same );
    double residual ( const gp_Trsf& tr ) const override;
};



class InclinedPlanes
    : public Condition
{
    DatumPtr pl_org_,  pl_targ_;
    ScalarPtr angle_;
    InclinedPlanes(const InclinedPlanes&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((pl_org_,pl_targ_,angle_));
#endif
    CLONEABLE(InclinedPlanes);
    InclinedPlanes ( DatumPtr pl_org, DatumPtr pl_targ, ScalarPtr angle );
    double residual ( const gp_Trsf& tr ) const override;
};




class Coaxial
    : public Condition
{
    DatumPtr ax_org_,  ax_targ_;
    bool inv_;
    Coaxial(const Coaxial&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((ax_org_,  ax_targ_));
#endif
    CLONEABLE(Coaxial);
    Coaxial ( DatumPtr ax_org, DatumPtr ax_targ, bool inv=false );
    double residual ( const gp_Trsf& tr ) const override;
};




class PointInPlane
    : public Condition
{
    VectorPtr p_org_;
    DatumPtr pl_targ_;
    PointInPlane(const PointInPlane&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((p_org_,pl_targ_));
#endif
    CLONEABLE(PointInPlane);
    PointInPlane ( VectorPtr p_org, DatumPtr pl_targ );
    double residual ( const gp_Trsf& tr ) const override;
};




class PointOnAxis
    : public Condition
{
    VectorPtr p_org_;
    DatumPtr ax_targ_;
    PointOnAxis(const PointOnAxis&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS_NOINVALIDATE((p_org_, ax_targ_));
#endif
    CLONEABLE(PointOnAxis);
    PointOnAxis ( VectorPtr p_org, DatumPtr ax_targ );
    double residual ( const gp_Trsf& tr ) const override;
};




typedef std::shared_ptr<Condition> ConditionPtr;
typedef std::vector<ConditionPtr> ConditionList;




class RefPlace
    : public DerivedFeature
{
    ConditionList conditions_;

    std::shared_ptr<gp_Trsf> trsf_;

    RefPlace(const RefPlace&o, TreeCloneMap& tcm);
    RefPlace ( ConstFeaturePtr m, const gp_Ax2& cs );
    RefPlace ( ConstFeaturePtr m, ConditionList conditions );

    size_t calcHash() const override;
    void build() override;

public:
    CLONEABLE(RefPlace);
    declareType ( "RefPlace" );
#ifndef SWIG
    DEPENDS_W_BASE(DerivedFeature, (conditions_));
#endif
    CREATE_FUNCTION(RefPlace);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    bool isTransformationFeature() const override
    {
        return true;
    }
    gp_Trsf transformation() const override;
};




}
}

#endif // INSIGHT_CAD_REFPLACE_H
