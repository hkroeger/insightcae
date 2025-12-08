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

#ifndef INSIGHT_CAD_ARC_H
#define INSIGHT_CAD_ARC_H

#include "cadparameters.h"
#include "constrainedsketchentity.h"
#include "cadfeatures/singleedgefeature.h"
#include "constrainedsketchentities/sketchpoint.h"
#include <memory>

namespace insight 
{
namespace cad
{

        
    
class Arc
    : public SingleEdgeFeature,
      public ConstrainedSketchEntity
{
public:
    enum ThirdVectorType
    {
        NormalTimesRadius, IntermediatePoint, P0Tangent, P1Tangent
    };

private:
    VectorPtr p0_;
    VectorPtr p1_;

    ThirdVectorType avType_;
    VectorPtr av_;

    size_t calcHash() const override;
    void build() override;

    Arc(const Arc&o, TreeCloneMap& tcm);
    Arc (
        VectorPtr p0,
        ThirdVectorType avType, VectorPtr av,
        VectorPtr p1,
        const std::string& layerName=std::string() );

public:
    declareType ( "Arc" );
#ifndef SWIG
    DEPENDS((p0_, p1_, av_));
#endif
    CLONEABLE(Arc);

    static std::shared_ptr<Arc> create(
        VectorPtr p0,
        ThirdVectorType avType, VectorPtr av,
        VectorPtr p1,
        const std::string& layerName=std::string() );

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    VectorPtr start() const override { return p0_; }
    VectorPtr end() const  override { return p1_; }

    bool isSingleOpenWire() const override;

    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        const ConstrainedSketchParametersDelegate& pd );

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
    dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    Handle_Geom_TrimmedCurve calcArc() const;

    bool isInside( SelectionRect r) const override;
    bool pointIsOnLine(const arma::mat& p) const;
    arma::mat projectOntoLine(const arma::mat& p) const;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const Arc& other);

    ConstrainedSketchEntityPtr clone() const override;

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;
};




class ArcCenterPoint
: public SketchPoint
{
    double angle_;
    std::weak_ptr<Arc> arc_;

    ArcCenterPoint(DatumPtr plane, double angle, const std::string& layerName = std::string());

    friend class Arc;
    void linktoArc(std::weak_ptr<Arc> arc);

    arma::mat calcXYFromArc() const;

public:
    declareType("ArcCenterPoint");

    CREATE_FUNCTION(ArcCenterPoint);

    void setCoords2D(double x, double y) override;
    arma::mat coords2D() const override;
    arma::mat normalTimesAngle() const;

    int nDoF() const override;
    double getDoFValue(unsigned int iDoF) const override;
    void setDoFValue(unsigned int iDoF, double value) override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        const ConstrainedSketchParametersDelegate& pd );



    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const ArcCenterPoint& other);

    ConstrainedSketchEntityPtr clone() const override;
};




}
}

#endif // INSIGHT_CAD_ARC_H
