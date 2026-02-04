#include "cadgeometryparameter.h"

#include "base/hierarchicalelement.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/parameters/pathparameter.h"
#include "base/filecontainer.h"
#include <memory>
#include <sstream>

#include "cadfeature.h"
#include "cadfeatures/modelfeature.h"
#include "cadtypes.h"
#include "datum.h"
#include "cadmodel.h"

#include "cadfeatures/stl.h"
#include "cadfeatures/importsolidmodel.h"
#include "BRepTools.hxx"


namespace insight {


defineType(CADGeometryParameter);
addParameterFactories(CADGeometryParameter);




cad::FeaturePtr CADGeometryParameter::createOrReadFeature()
{
    if (auto *fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto fn=(**fcp).accessibleFilePath();
        std::string ext=fn.extension().string();
        if ( (ext==".stl") || (ext==".stlb") )
        {
            return cad::STL::create( fn );
        }
        else
            return cad::Import::create( fn );
    }
    else if (auto *ftp=boost::get<cad::FeaturePtr>(&value_))
    {
        return *ftp;
    }
    else if (auto *sp=boost::get<std::string>(&value_))
    {
        cad::ModelPtr m;
        parseISCADModel(*sp, m.get());
        return cad::ModelFeature::create(m);
    }
    else
        throw insight::UnhandledSelection();

    return cad::FeaturePtr();
}






CADGeometryParameter::CADGeometryParameter (
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order  )
: CADGeometryParameterBase(
          description, isHidden, isExpert, isNecessary, order)
{}




CADGeometryParameter::CADGeometryParameter (
    value_type value,
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order  )
: CADGeometryParameterBase(
          description, isHidden, isExpert, isNecessary, order),
    value_(value)
{
}




CADGeometryParameter::CADGeometryParameter (
    const boost::filesystem::path& fileName,
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order  )
    : CADGeometryParameterBase(
          description, isHidden, isExpert, isNecessary, order)
{
    setGeometryFile(fileName);
}




bool CADGeometryParameter::isDifferent(const Parameter& p) const
{
    if (const auto *pp = dynamic_cast<const CADGeometryParameter*>(&p))
    {
        if (pp->value_.type()!=value_.type())
            return true;

        if (auto *fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
        {
            auto *ofcp=boost::get<std::shared_ptr<FileContainer> >(&pp->value_);
            return (**fcp).isDifferent(**ofcp);
        }
        else if (auto *ftp=boost::get<cad::FeaturePtr>(&value_))
        {
            auto *oftp=boost::get<cad::FeaturePtr>(&pp->value_);
            return !((**ftp)==(**oftp));
        }
        else if (auto *sp=boost::get<std::string>(&value_))
        {
            auto *osp=boost::get<std::string>(&pp->value_);
            return !((*sp)==(*osp));
        }
        return true;
    }
    else
        return true;
}




std::string CADGeometryParameter::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        return SimpleLatex( (**fcp).filePath().string() ).toLaTeX();
    }
    else if (boost::get<cad::FeaturePtr>(&value_))
    {
        return "CAD geometry";
    }
    else if (auto* scr=boost::get<std::string>(&value_))
    {
        return "CAD script";
    }
    return std::string();
}




std::string CADGeometryParameter::plainTextRepresentation(int indent) const
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        return (**fcp).filePath().string();
    }
    else if (boost::get<cad::FeaturePtr>(&value_))
    {
        return "CAD geometry";
    }
    else if (auto* scr=boost::get<std::string>(&value_))
    {
        return "CAD script";
    }
    return std::string();
}




void CADGeometryParameter::resolveRelativePaths(const boost::filesystem::path& baseDirectory)
{
    baseDirectory_=baseDirectory;
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        fc.resolveRelativePath(baseDirectory);
    }
}




bool CADGeometryParameter::isPacked() const
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        return fc.hasFileContent();
    }
    return false;
}




void CADGeometryParameter::pack()
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        auto lfp=fc.expandedFilePath();
        if (boost::filesystem::exists(lfp))
            fc.replaceContent(lfp);
    }
}



void CADGeometryParameter::unpack(const boost::filesystem::path& basePath)
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        fc.accessibleFilePath(true, basePath); // triggers unpack
    }
}



void CADGeometryParameter::clearPackedData()
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        fc.clearPackedData();
    }
}


/**
   * @brief fileName
   * @return returns the file name component only
   */
boost::optional<boost::filesystem::path> CADGeometryParameter::filePath() const
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        return fc.filePath();
    }
    return boost::none;
}


boost::optional<boost::filesystem::path> CADGeometryParameter::accessibleFilePath(
    bool unpackIfNoLocalCopy,
    boost::optional<boost::filesystem::path> overrideBaseDirectory ) const
{
    if (auto* fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto& fc=**fcp;
        return fc.accessibleFilePath(unpackIfNoLocalCopy, overrideBaseDirectory);
    }
    return boost::none;
}

bool CADGeometryParameter::isValid() const
{
    if (auto *fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        if (*fcp)
        {
            return (**fcp).isValid();
        }
    }
    else if (auto *ftp=boost::get<cad::FeaturePtr>(&value_))
    {
        return bool(*ftp);
    }
    else if (auto *scr = boost::get<std::string>(&value_))
    {
        return !scr->empty();
    }

    return false;
}

cad::FeaturePtr CADGeometryParameter::geometry() const
{
    return geometry_.ptr();
}

void CADGeometryParameter::setGeometryFile(const boost::filesystem::path& fn)
{
    geometry_.reset();

    auto val=fn;
    if (!fn.empty() && baseDirectory_)
    {
        val=boost::filesystem::make_relative(
            *baseDirectory_, fn);
    }
    value_=std::make_shared<FileContainer>(val, baseDirectory_);

    triggerValueChanged();
}

void CADGeometryParameter::setGeometry(cad::FeaturePtr geom)
{
    geometry_.reset();

    value_=geom;

    triggerValueChanged();
}


void CADGeometryParameter::setScript(const std::string& scr)
{
    geometry_.reset();

    value_=scr;

    triggerValueChanged();
}

rapidxml::xml_node<>* CADGeometryParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps ) const
{
    insight::CurrentExceptionContext ex(
        insight::VerbosityLevel::Loops,
        "appending geometry %s to node %s", name.c_str(), node.name() );

    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, outProps);

    std::string sourceType;
    if (auto *fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        sourceType="file";
        (**fcp).appendToNode( doc, *child, "fileName" );
    }
    else if (auto *ftp=boost::get<cad::FeaturePtr>(&value_))
    {
        sourceType="feature";
        std::ostringstream os;
        BRepTools::Write((**ftp).shape(), os);

        child->append_attribute(
            doc.allocate_attribute
            (
                doc.allocate_string("featureBREP"),
                base64_encode(doc, os.str())
                ));
    }
    else if (auto *scr = boost::get<std::string>(&value_))
    {
        sourceType="script";
        appendAttribute(doc, *child, "script", *scr);
    }
    else
        throw insight::UnhandledSelection();


    appendAttribute(doc, *child, "source", sourceType);

    return child;
}




CADGeometryParameter::CADGeometryParameter (
    const rapidxml::xml_node<> & node)
: CADGeometryParameterBase(node)
{
    readFromNode("", node);
}




const rapidxml::xml_node<>* CADGeometryParameter::readFromNode(
    const std::string& name,
    const rapidxml::xml_node<>& node)
{
    auto* child = Parameter::readFromNode(name, node);
    if (child)
    {
        geometry_.reset();

        auto sourceType = getMandatoryAttribute(*child, "source");
        if (sourceType=="file")
        {
            auto fc=std::make_shared<FileContainer>();
            fc->readFromNode(*child, "fileName");
            value_=fc;
        }
        else if (sourceType=="feature")
        {
            if (auto* a = node.first_attribute("featureBREP"))
            {
                std::shared_ptr<std::string> buf;
                base64_decode(a->value(), a->value_size(), buf);
                std::istringstream is(*buf);
                TopoDS_Shape shape;
                BRep_Builder bb;
                BRepTools::Read(shape, is, bb);
                value_=cad::Import::create(shape);
            }
            else
                throw insight::Exception("expected node 'featureBREP' not found");
        }
        else if (sourceType=="script")
        {
            value_=getMandatoryAttribute(*child, "script");
        }
        else
            throw insight::UnhandledSelection(
                "unrecognized geometry option: "+sourceType
                +". Expected either 'fileName', 'feature' or 'script'");

        triggerValueChanged();
    }
    return child;
}




std::unique_ptr<hierarchicalData::Element> CADGeometryParameter::clone() const
{
    auto p= std::make_unique<CADGeometryParameter>(
        value_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );

    if (baseDirectory_)
        p->resolveRelativePaths(*baseDirectory_);

    return p;
}




void CADGeometryParameter::assignFrom(const Element& e)
{
    auto &og = dynamic_cast<const CADGeometryParameter&>(e);

    geometry_.reset();

    baseDirectory_=og.baseDirectory_;

    if (auto *fcp=boost::get<std::shared_ptr<FileContainer> >(&value_))
    {
        auto ofc=boost::get<std::shared_ptr<FileContainer> >(og.value_);
        (**fcp) = *ofc;
    }
    else if (auto *ftp=boost::get<cad::FeaturePtr>(&value_))
    {
        auto oft=boost::get<cad::FeaturePtr>(og.value_);
        *ftp=oft;
    }
    else if (auto *scr = boost::get<std::string>(&value_))
    {
        auto& oscr = boost::get<std::string>(og.value_);
        value_=oscr;
    }
    else
        throw insight::UnhandledSelection();

    triggerValueChanged();
}




bool CADGeometryParameter::isEqual(const Element& op) const
{
    if (auto *oa = dynamic_cast<const CADGeometryParameter*>(&op))
    {
        return !isDifferent(*oa);
    }
    else
        return false;
}




int CADGeometryParameter::nChildren() const
{
    return 0;
}




std::shared_ptr<CADGeometryParameter> make_geometryFile(
    const boost::filesystem::path &path )
{
  return std::make_shared<CADGeometryParameter>(
        path, "temporary geometry file");
}



std::shared_ptr<CADGeometryParameter> make_geometryFile(
    std::shared_ptr<PathParameter> path )
{
    return std::make_shared<CADGeometryParameter>(
        std::dynamic_pointer_cast<FileContainer>(path),
        "temporary geometry file" );
}



std::shared_ptr<CADGeometryParameter> make_geometryFile(
   cad::FeaturePtr feat )
{
    return std::make_shared<CADGeometryParameter>(
        feat, "temporary geometry");
}




std::shared_ptr<CADGeometryParameter> make_geometryFile(
    vtkSmartPointer<vtkPolyData> pd )
{
    return std::make_shared<CADGeometryParameter>(
        cad::STL::create(pd),
        "temporary STL geometry" );
}




} // namespace insight
