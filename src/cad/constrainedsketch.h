#ifndef INSIGHT_CAD_CONSTRAINEDSKETCH_H
#define INSIGHT_CAD_CONSTRAINEDSKETCH_H


#include "base/cppextensions.h"
#include "sketch.h"
#include "base/exception.h"
#include "base/parameterset.h"
#include "constrainedsketchentities/sketchpoint.h"
#include "constrainedsketchentities/externalreference.h"
#include <memory>


class IQParameterSetModel;


namespace insight {
namespace cad {




class ConstrainedSketchScriptBuffer
{
    std::set<int> entitiesPresent_;
    std::vector<std::string> script_, layerProps_;

public:
    void insertCommandFor(int entityLabel, const std::string& cmd);
    void appendLayerProp(const std::string& cmd);
    void write(std::ostream& os);
};




struct LayerProperties
    : public insight::ParameterSet
{
    using insight::ParameterSet::ParameterSet;

    static std::unique_ptr<LayerProperties> create();
    static std::unique_ptr<LayerProperties> create(
        const ParameterSet& parameters );

    std::unique_ptr<LayerProperties> cloneLayerProperties() const
    {
        return create(*this);
    }
};





/**
 * @brief The ConstrainedSketchEntityParametersDelegate class
 * manages creation and editing of a certain sketch
 */
class ConstrainedSketchParametersDelegate
{
public:
    /**
     * @brief createDefaultParameters
     * @param e
     * @return
     * the parameters associated with the given entity
     */
    virtual void
    changeDefaultParameters(ConstrainedSketchEntity& e) const;

    /**
     * @brief createDefaultLayerProperties
     * @param e
     * @return
     * the parameters associated with the given layer
     */
    virtual std::unique_ptr<LayerProperties>
    createDefaultLayerProperties(const std::string& layerName) const;
};


extern std::shared_ptr<ConstrainedSketchParametersDelegate> noParametersDelegate;



class ConstrainedSketchPresentationDelegate
{
public:
    declareType("ConstrainedSketchPresentationDelegate");

    // typedef
    //     insight::StaticFunctionTable<
    //         std::shared_ptr<ConstrainedSketchPresentationDelegate>
    //         >
    //         DelegateFactories;

    declareStaticFunctionTable2(
        DelegateFactories, delegates,
        std::shared_ptr<ConstrainedSketchPresentationDelegate> );

    template<class Instance>
    struct Add
        : public insight::cad::ConstrainedSketchPresentationDelegate
                ::DelegateFactories::Add<Instance>
    {
        Add()
            : insight::cad::ConstrainedSketchPresentationDelegate
                ::DelegateFactories::Add<Instance>(
                    &insight::cad::ConstrainedSketchPresentationDelegate
                  ::delegates,
                    &std::make_shared<Instance>
                )
        {}
    };

    /**
     * @brief setupSketchEditorParameterSetModel
     * @param e
     * @return
     * an IQParameterSetModel for editing the parameters of the given entity.
     * The model may be enriched with additional data for the presentation,
     * e.g. base points for vectors which are required during editing
     */
    virtual IQParameterSetModel*
    setupSketchEntityParameterSetModel(
        const ConstrainedSketchEntity& e) const;

    virtual IQParameterSetModel*
    setupLayerParameterSetModel(
        const std::string& layerName, const LayerProperties& e) const;

    /**
     * @brief setEntityAppearance
     * modifies the VTK actor
     * @param e
     * @param actprops
     */
    virtual void setEntityAppearance(
        const ConstrainedSketchEntity& e, vtkProperty* actprops) const;
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

    static const std::string defaultLayerName;

#ifndef SWIG
    typedef boost::signals2::signal<void(GeometryMap::key_type)> GeometryEditSignal;
    GeometryEditSignal geometryAdded, geometryAboutToBeRemoved, geometryRemoved, geometryChanged;
#endif

private:
    DatumPtr pl_;
    std::observer_ptr<Parameter> propertiesParent_;

    // ID is key.
    // storage of ID is required for proper update during parameter refresh (via parse from script)
    GeometryMap geometry_;

    SolverSettings solverSettings_;

    std::map<std::string, std::unique_ptr<LayerProperties> > layerProperties_;

    ConstrainedSketch( DatumPtr pl, const ConstrainedSketchParametersDelegate& pd );
    ConstrainedSketch( const ConstrainedSketch& other );

    size_t calcHash() const override;
    void build() override;

public:
    // required to make boost::adaptors::index work
    using iterator = typename GeometryMap::iterator;
    using const_iterator = typename GeometryMap::const_iterator;
    using value_type = typename GeometryMap::value_type;

    declareType("ConstrainedSketch");

    CREATE_COPY_FUNCTION(ConstrainedSketch);
    CREATE_FUNCTION(ConstrainedSketch);

    void setParentParameter(Parameter* p);
    Parameter* parentParameter() const;

    static std::shared_ptr<ConstrainedSketch> createFromStream(
        DatumPtr pl,
        std::istream& is,
        const ConstrainedSketchParametersDelegate& pd);

    void readFromStream(
        std::istream& is,
        const ConstrainedSketchParametersDelegate& pd );

    const DatumPtr& plane() const;
    VectorPtr sketchPlaneNormal() const;
    arma::mat p3Dto2D(const arma::mat& p3d) const;

    GeometryMap::key_type findUnusedID(int direction=1) const;

    std::set<ConstrainedSketchEntityPtr> findConnected(
        ConstrainedSketchEntityPtr theEntity,
        std::set<ConstrainedSketchEntityPtr> *subset = nullptr) const;

    // addition
    GeometryMap::key_type insertGeometry(
        ConstrainedSketchEntityPtr geomEntity,
        boost::variant<boost::blank,GeometryMap::key_type> key = boost::blank() );
    GeometryMap::key_type setExternalReference(
        std::shared_ptr<ExternalReference> extRef,
        boost::variant<boost::blank,GeometryMap::key_type> key = boost::blank() );

    // removal
    void eraseGeometry(GeometryMap::key_type geomEntityId);
    void eraseGeometry(ConstrainedSketchEntityPtr geomEntity);
    void clear();

    size_t size() const;
    GeometryMap::const_iterator findGeometry(ConstrainedSketchEntityPtr geomEntity) const;
    GeometryMap::const_iterator begin() const;
    GeometryMap::const_iterator end() const;
    GeometryMap::const_iterator cbegin() const;
    GeometryMap::const_iterator cend() const;

    template<class T = ConstrainedSketchEntity>
    std::shared_ptr<T> get(GeometryMap::key_type geomEntityId) const
    {
        auto i=geometry_.find(geomEntityId);
        if (i==geometry_.end()) return nullptr;
        return std::dynamic_pointer_cast<T>(i->second);
    }

    std::set<ConstrainedSketchEntityPtr> filterGeometryByParameters(
        std::function<bool(const ParameterSet& geomPS)> filterFunction
        ) const;

    cad::FeaturePtr layerGeometry(const std::string& layerName) const;

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

    std::set<std::string> usedLayerNames() const;
    std::set<std::string> layerNames() const;

    bool hasLayer(const std::string& layerName) const;
    bool layerIsUsed(const std::string& layerName) const;

    void addLayer(
        const std::string& layerName,
        const ConstrainedSketchParametersDelegate& pd );

    const LayerProperties& layerProperties(
        const std::string& layerName ) const;

    void setLayerProperties(
        const std::string& layerName,
        const ParameterSet& ps);

    void parseLayerProperties(
        const std::string& layerName,
        const boost::optional<std::string>& ps,
        const ConstrainedSketchParametersDelegate& pd );

    void removeLayer(const std::string& layerName);

    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
    entitiesInsideRect( double x1, double y1, double x2, double y2 ) const;
};




typedef std::shared_ptr<ConstrainedSketch> ConstrainedSketchPtr;





} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CONSTRAINEDSKETCH_H
