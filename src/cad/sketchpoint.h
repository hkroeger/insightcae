#ifndef INSIGHT_CAD_SKETCHPOINT_H
#define INSIGHT_CAD_SKETCHPOINT_H


#include "constrainedsketchgeometry.h"



namespace insight {
namespace cad {



class SketchPoint
    : public insight::cad::Vector,
      public ConstrainedSketchEntity
{

    DatumPtr plane_;
    double x_, y_;

public:
    declareType("SketchPoint");

    SketchPoint(DatumPtr plane, const arma::mat& xy, const std::string& layerName = std::string());
    SketchPoint(DatumPtr plane, double x, double y, const std::string& layerName = std::string());
    void setCoords2D(double x, double y);
    arma::mat coords2D() const;
    arma::mat value() const override;

    int nDoF() const override;
    double getDoFValue(unsigned int iDoF) const override;
    void setDoFValue(unsigned int iDoF, double value) override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(ConstrainedSketchGrammar& ruleset, MakeDefaultGeometryParametersFunction mdpf);

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;
    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const SketchPoint& other);
};


typedef std::shared_ptr<SketchPoint> SketchPointPtr;


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SKETCHPOINT_H
