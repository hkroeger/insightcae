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

    typedef std::map<int, ConstrainedSketchEntityPtr> GeometryMap;

private:
    DatumPtr pl_;

    // ID is key.
    // storage of ID is required for proper update during parameter refresh (via parse from script)
    GeometryMap geometry_;

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

    void readFromStream(
        std::istream& is,
        const ParameterSet& geomPS = insight::ParameterSet() );

    const DatumPtr& plane() const;
    VectorPtr sketchPlaneNormal() const;

    GeometryMap::key_type findUnusedID() const;
    void insertGeometry(ConstrainedSketchEntityPtr geomEntity, GeometryMap::key_type key=-1);

    void eraseGeometry(GeometryMap::key_type geomEntityId);
    void eraseGeometry(ConstrainedSketchEntityPtr geomEntity);

    size_t size() const;
    void clear();
    GeometryMap::const_iterator findGeometry(ConstrainedSketchEntityPtr geomEntity) const;
    GeometryMap::const_iterator begin() const;
    GeometryMap::const_iterator cbegin() const;
    GeometryMap::const_iterator end() const;
    GeometryMap::const_iterator cend() const;

    template<class T>
    std::shared_ptr<T> get(GeometryMap::key_type geomEntityId) const
    {
        auto i=geometry_.find(geomEntityId);
        if (i==geometry_.end()) return nullptr;
        return std::dynamic_pointer_cast<T>(i->second);
    }

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

    arma::mat sketchBoundingBox() const;

    std::set<std::string> layers() const;

};




typedef std::shared_ptr<ConstrainedSketch> ConstrainedSketchPtr;





} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CONSTRAINEDSKETCH_H
