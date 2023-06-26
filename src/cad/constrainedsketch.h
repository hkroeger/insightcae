#ifndef INSIGHT_CAD_CONSTRAINEDSKETCH_H
#define INSIGHT_CAD_CONSTRAINEDSKETCH_H


#include "sketch.h"
#include "sketchpoint.h"

namespace insight {
namespace cad {




class ConstrainedSketchScriptBuffer
{
    std::set<int> entitiesPresent_;
    std::vector<std::string> script_;

public:
    void insertCommandFor(int entityLabel, const std::string& cmd);
    void write(std::ostream& os);
};





class ConstrainedSketch
    : public Feature
{
public:
    enum SolverType
    {
        rootND =0,
        minimumND =1
    };

    struct SolverSettings
    {
        SolverType solver_;
        double tolerance_;
        double relax_;
        int maxIter_;
    };

private:
    DatumPtr pl_;
    std::set<ConstrainedSketchEntityPtr> geometry_;

    SolverSettings solverSettings_;

    ConstrainedSketch( DatumPtr pl );

    size_t calcHash() const override;
    void build() override;

public:
    declareType("ConstrainedSketch");

    CREATE_FUNCTION(ConstrainedSketch);
    static std::shared_ptr<ConstrainedSketch> createFromStream(
        DatumPtr pl,
        std::istream& is,
        const ParameterSet& geomPS = insight::ParameterSet() );

    const DatumPtr& plane() const;

    std::set<ConstrainedSketchEntityPtr>& geometry();
    const std::set<ConstrainedSketchEntityPtr>& geometry() const;

    std::set<ConstrainedSketchEntityPtr> filterGeometryByParameters(
        std::function<bool(const ParameterSet& geomPS)> filterFunction
        );

    void operator=(const ConstrainedSketch& o);

    const SolverSettings& solverSettings() const;
    void changeSolverSettings(const SolverSettings& ss);

    void resolveConstraints(
        std::function<void(void)> perIterationCallback = std::function<void(void)>(),
        ProgressDisplayer& progress = consoleProgressDisplayer );

    static void insertrule(parser::ISCADParser& ruleset);

    void generateScript(std::ostream& os) const;

    std::string generateScriptCommand() const override;

};




typedef std::shared_ptr<ConstrainedSketch> ConstrainedSketchPtr;





} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CONSTRAINEDSKETCH_H
