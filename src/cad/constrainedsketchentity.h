#ifndef INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H
#define INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H

#include <memory>
#include <string>
#include "base/cppextensions.h"

#include "base/parameterset.h"
#include "cadtypes.h"

#include "constrainedsketchgrammar.h"
#include "boost/spirit/include/qi.hpp"


namespace insight {
namespace cad {

class ConstrainedSketch;
class ConstrainedSketchEntity;
class ConstrainedSketchScriptBuffer;

typedef
    std::shared_ptr<ConstrainedSketchEntity>
        ConstrainedSketchEntityPtr;


class ConstrainedSketchEntity
{
    std::unique_ptr<insight::ParameterSet> parameters_, defaultParameters_;

    std::string layerName_;

public:
    declareType("ConstrainedSketchEntity");

    declareStaticFunctionTableWithArgs(
        addParserRule,
        void,
        LIST(ConstrainedSketchGrammar&, const ConstrainedSketchParametersDelegate&),
        LIST(ConstrainedSketchGrammar& ruleset, const ConstrainedSketchParametersDelegate& pd));

    ConstrainedSketchEntity(const std::string& layerName = std::string());
    virtual ~ConstrainedSketchEntity();

    virtual int nDoF() const;
    virtual double getDoFValue(unsigned int iDoF) const;
    virtual void setDoFValue(unsigned int iDoF, double value);

    const std::string& layerName() const;
    void setLayerName(const std::string& layerName);

    virtual int nConstraints() const;
    virtual double getConstraintError(unsigned int iConstraint) const;

    virtual void scaleSketch(double scaleFactor) =0;

    virtual size_t hash() const;

    const insight::ParameterSet& parameters() const;
    insight::ParameterSet& parametersRef();
    const insight::ParameterSet& defaultParameters() const;


    void changeDefaultParameters(
        const insight::ParameterSet& ps);

    void parseParameterSet(
        const std::string& s,
        const boost::filesystem::path& inputFileParentPath);

    std::string pointSpec(
        insight::cad::VectorPtr p,
        ConstrainedSketchScriptBuffer &script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const;

    std::string parameterString() const;

    virtual void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const =0;

    virtual std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const =0;

    bool dependsOn(const std::weak_ptr<ConstrainedSketchEntity>& entity) const;

    virtual void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) =0;

    virtual void operator=(const ConstrainedSketchEntity& other);

    virtual std::vector<vtkSmartPointer<vtkProp> > createActor() const =0;

    virtual ConstrainedSketchEntityPtr clone() const =0;

};




} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H
