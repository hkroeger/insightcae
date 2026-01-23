#ifndef INSIGHT_CADGEOMETRYPARAMETER_H
#define INSIGHT_CADGEOMETRYPARAMETER_H

#include "base/hierarchicalelement.h"
#include "cadtypes.h"
#include "base/parameter.h"
#include "base/parameterset.h"
#include "base/filecontainer.h"




namespace insight {




class CADGeometryParameterBase
        : public Parameter
{
public:
    using Parameter::Parameter;

    virtual cad::FeaturePtr geometry() const =0;
};




class CADGeometryParameter
: public CADGeometryParameterBase
{

protected:
    typedef boost::variant<
     std::shared_ptr<FileContainer>, // from file
     std::string,     // cad script
     cad::FeaturePtr // directly supplied geometry
        > value_type;

    value_type value_;

    /**
   * @brief baseDirectory_
   * for relative paths: base directory for resolution,
   * stored here as well as in filecontainer, since content might be changed
   * to file after base directory had been broadcast
   */
    boost::optional<boost::filesystem::path> baseDirectory_;

    cad::FeaturePtr createOrReadFeature();
    OnDemand<cad::Feature> geometry_{
        std::bind(&CADGeometryParameter::createOrReadFeature, this)
    };

public:
    declareType ( "cadgeometry" );

    CADGeometryParameter (const rapidxml::xml_node<> & node);

    CADGeometryParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0  );

    CADGeometryParameter (
        value_type value,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0  );

    CADGeometryParameter (
        const boost::filesystem::path& value,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0  );
    bool isDifferent(const Parameter& p) const override;

    inline const boost::optional<boost::filesystem::path> baseDirectory() const
    {
        return baseDirectory_;
    }

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;
    std::string plainTextRepresentation(int indent) const override;

    void resolveRelativePaths(const boost::filesystem::path& baseDirectory) override;
    bool isPacked() const override;
    void pack() override;
    void unpack(const boost::filesystem::path& basePath) override;
    void clearPackedData() override;



    boost::optional<boost::filesystem::path> filePath() const;

    boost::optional<boost::filesystem::path> accessibleFilePath(
        bool unpackIfNoLocalCopy=true,
        boost::optional<boost::filesystem::path> overrideBaseDirectory
        = boost::optional<boost::filesystem::path>() ) const;

    bool isValid() const;
    cad::FeaturePtr geometry() const override;

    void setGeometryFile(const boost::filesystem::path& fn);
    void setGeometry(cad::FeaturePtr geom);
    void setScript(const std::string& scr);

    rapidxml::xml_node<>* appendToNode(
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const OutputProperties& outProps ) const override;

    const rapidxml::xml_node<>* readFromNode(
        const std::string& name,
        const rapidxml::xml_node<>& node) override;

    std::unique_ptr<Element> clone() const override;

    void assignFrom(const Element& e) override;
    bool isEqual(const Element& op) const override;

    int nChildren() const override;
};


std::shared_ptr<CADGeometryParameter> make_geometryFile(
    const boost::filesystem::path& path );

std::shared_ptr<CADGeometryParameter> make_geometryFile(
    std::shared_ptr<PathParameter> path );

std::shared_ptr<CADGeometryParameter> make_geometryFile(
    cad::FeaturePtr feat );

std::shared_ptr<CADGeometryParameter> make_geometryFile(
    vtkSmartPointer<vtkPolyData> pd );

} // namespace insight

#endif // INSIGHT_CADGEOMETRYPARAMETER_H
