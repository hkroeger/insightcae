#include "pathparameter.h"

#include <streambuf>
#include <ostream>
#include <memory>

#include "base/cppextensions.h"
#include "boost/filesystem/operations.hpp"
#include "vtkSTLWriter.h"

#include "base/tools.h"
#include "base/rapidxml.h"

namespace insight
{




defineType(PathParameter);
addParameterFactories(PathParameter);




void PathParameter::signalContentChange()
{
    valueChanged();
}



PathParameter::PathParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{}




PathParameter::PathParameter(
    const boost::filesystem::path& value, const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
: Parameter(description, isHidden, isExpert, isNecessary, order),
  FileContainer(value)
{}




PathParameter::PathParameter(
    const FileContainer& fc, const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
 :  Parameter(description, isHidden, isExpert, isNecessary, order),
    FileContainer(fc)
{}


bool PathParameter::isDifferent(const Parameter& p) const
{
  if (const auto *pp = dynamic_cast<const PathParameter*>(&p))
  {
    if (pp->fileName()!=fileName())
      return true;

    if (isPacked())
    {
      if (!pp->isPacked())
        return true;

      if (!(pp->contentModificationTime()==contentModificationTime()))
        return true;

      if (pp->contentBufferSize()!=contentBufferSize())
        return true;
    }

    return false;
  }
  else
    return true;
}


std::string PathParameter::latexRepresentation(
    const std::string&,
    int,
    const FileStorageInfo& ) const
{
    return SimpleLatex( fileName().string() ).toLaTeX();
}




std::string PathParameter::plainTextRepresentation(int /*indent*/) const
{
  return SimpleLatex( fileName().string() ).toPlainText();
}




void PathParameter::resolveRelativePaths(const boost::filesystem::path &baseDirectory)
{
    FileContainer::resolveRelativePath(baseDirectory);
    Parameter::resolveRelativePaths(baseDirectory);
}




bool PathParameter::isPacked() const
{
  return hasFileContent();
}




void PathParameter::pack()
{
    auto lfp=localFilePath();
    if (boost::filesystem::exists(lfp))
        replaceContent(lfp);
}




void PathParameter::unpack(const boost::filesystem::path &basePath)
{
    filePath(true, basePath); // triggers unpack
}




void PathParameter::clearPackedData()
{
  FileContainer::clearPackedData();
}




boost::filesystem::path
PathParameter::filePath(
    bool unpackIfNoLocalCopy,
    boost::optional<boost::filesystem::path> overrideBaseDirectory ) const
{
    boost::filesystem::path fp = localFilePath();

    if (!boost::filesystem::exists(fp) && unpackIfNoLocalCopy && hasFileContent())
    {
        auto baseDir = baseDirectory();

        if (overrideBaseDirectory)
            baseDir=*overrideBaseDirectory;

        if (!baseDir)
        {
          fp = GlobalTemporaryDirectory::path()/fileName();
        }
        else
        {
            fp = *baseDir / "embeddedFiles";
            if (!fileName().parent_path().empty())
            {
                auto dirHash = std::hash<std::string>()(
                    fileName().parent_path().string());
                fp /= toString(dirHash);
            }
            fp /= fileName();
        }
    }

    if (unpackIfNoLocalCopy && hasFileContent())
    {
        if (!boost::filesystem::exists(fp))
        {
            copyTo(fp, true);
        }
    }

    return fp;
}









rapidxml::xml_node<>* PathParameter::appendToNode
(
  const std::string& name,
  rapidxml::xml_document<>& doc,
  rapidxml::xml_node<>& node
) const
{
    insight::CurrentExceptionContext ex(
        insight::VerbosityLevel::Loops,
        "appending path %s to node %s", name.c_str(), node.name() );

    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node);

    FileContainer::appendToNode( doc, *child );

    return child;

}




const rapidxml::xml_node<>*
PathParameter::readFromNode
(
  const std::string& name,
  const rapidxml::xml_node<>& node
)
{
  using namespace rapidxml;
  auto* child = Parameter::readFromNode(name, node);
  if (child)
  {
    FileContainer::readFromNode(*child);
    triggerValueChanged();
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
            ) % type() % name % fileName().c_str() ) );
  }
  return child;
}


PathParameter::PathParameter(const rapidxml::xml_node<> &node)
    : Parameter(node)
{
    FileContainer::readFromNode(node);
    triggerValueChanged();
}


std::unique_ptr<PathParameter> PathParameter::clonePathParameter() const
{
    auto pp=cloneAs<PathParameter>();
    return pp;
}

std::unique_ptr<hierarchicalData::Element> PathParameter::clone() const
{
    auto p= std::make_unique<PathParameter>(
        *this,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
    return p;
}



void PathParameter::assignFrom(const Element& e)
{
  auto& op=dynamic_cast<const PathParameter&>(e);
  FileContainer::operator=(op);
  Parameter::assignFrom(op);
}


bool PathParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const PathParameter*>(&op))
    {
        if (!FileContainer::operator==(*oa))
            return false;
        return true;
    }
    else
        return false;
}


int PathParameter::nChildren() const
{
  return 0;
}




std::shared_ptr<PathParameter> make_filepath(const boost::filesystem::path& path)
{
  return std::shared_ptr<PathParameter>(new PathParameter(path, "temporary file path"));
}




std::shared_ptr<PathParameter> make_filepath(const FileContainer& fc)
{
  return std::shared_ptr<PathParameter>(new PathParameter(fc, "temporary file path"));
}



std::shared_ptr<PathParameter> make_filepath(
    vtkSmartPointer<vtkPolyData> pd,
    const boost::filesystem::path& originalFilePath )
{
  TemporaryFile tf( originalFilePath.filename().stem().string()+"-%%%%%%.stl" );
  auto msw = vtkSmartPointer<vtkSTLWriter>::New();
  msw->SetInputData(pd);
  msw->SetFileTypeToBinary();
  msw->SetFileName(tf.path().string().c_str());
  msw->Update();
  return std::make_shared<PathParameter>(
        FileContainer(tf, originalFilePath),
        "temporary file path" );
}






defineType(DirectoryParameter);
addParameterFactories(DirectoryParameter);



DirectoryParameter::DirectoryParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(".", description, isHidden, isExpert, isNecessary, order)
{}

DirectoryParameter::DirectoryParameter(const boost::filesystem::path& value, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(value, description, isHidden, isExpert, isNecessary, order)
{}

std::string DirectoryParameter::latexRepresentation(
    const std::string&,
    int,
    const FileStorageInfo& ) const
{
    return std::string()
      + "{\\ttfamily "
      + SimpleLatex(
            fileName().string()
        ).toLaTeX()
      + "}";
}

void DirectoryParameter::pack()
{}

void DirectoryParameter::unpack(const boost::filesystem::path &basePath)
{}



rapidxml::xml_node<>* DirectoryParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node);
    FileContainer::appendToNode(doc, *child);
    return child;
}




const rapidxml::xml_node<>* DirectoryParameter::readFromNode
(
    const std::string& name,
    const rapidxml::xml_node<>& node
)
{
  using namespace rapidxml;
  auto* child = Parameter::readFromNode(name, node);
  if (child)
  {
    FileContainer::readFromNode(*child);
    triggerValueChanged();
  }
  else
  {
    insight::Warning(
             "No xml node found with type '%s' and name '%s', default value '%s' is used.",
              type().c_str(), name.c_str(), fileName().c_str()
        );
  }
  return child;
}

DirectoryParameter::DirectoryParameter(const rapidxml::xml_node<> &node)
    : PathParameter(node)
{
    FileContainer::readFromNode(node);
    triggerValueChanged();
}

bool DirectoryParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const DirectoryParameter*>(&op))
    {
        return PathParameter::isEqual(*oa);
    }
    else
        return false;
}




std::unique_ptr<hierarchicalData::Element> DirectoryParameter::clone() const
{
    auto p=std::make_unique<DirectoryParameter>(
        fileName(),
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
    return p;
}

std::unique_ptr<DirectoryParameter> DirectoryParameter::cloneDirectoryParameter() const
{
    auto pp=cloneAs<DirectoryParameter>();
    return pp;
}




}
