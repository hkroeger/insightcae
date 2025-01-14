#include "pathparameter.h"

#include <streambuf>
#include <ostream>
#include <memory>

#include "vtkSTLWriter.h"

#include "base/tools.h"
#include "base/rapidxml.h"

namespace insight
{




defineType(PathParameter);
addToFactoryTable(Parameter, PathParameter);




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
    bool isHidden, bool isExpert, bool isNecessary, int order,
    std::shared_ptr<std::string> binary_content
    )
: Parameter(description, isHidden, isExpert, isNecessary, order),
  FileContainer(value, binary_content)
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
    if (pp->originalFilePath()!=originalFilePath())
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


std::string PathParameter::latexRepresentation() const
{
    return SimpleLatex( originalFilePath_.string() ).toLaTeX();
}




std::string PathParameter::plainTextRepresentation(int /*indent*/) const
{
  return SimpleLatex( originalFilePath_.string() ).toPlainText();
}




bool PathParameter::isPacked() const
{
//  return FileContainer::isPacked();
  return hasFileContent();
}




void PathParameter::pack()
{
//  FileContainer::pack()
  replaceContent(originalFilePath_);
}




void PathParameter::unpack(const boost::filesystem::path &basePath)
{
  auto up = unpackFilePath(basePath);
  if (needsUnpack(up))
  {
      std::cout<<"unpacking "<<originalFilePath_<<" to "<<up<<std::endl;
      copyTo(up, true);
  }
//  FileContainer::unpack(basePath);
}




void PathParameter::clearPackedData()
{
  FileContainer::clearPackedData();
}




boost::filesystem::path PathParameter::filePath(boost::filesystem::path baseDirectory) const
{
  auto up=unpackFilePath(baseDirectory);

  if (needsUnpack(up))
  {
    copyTo(up, true);
  }

  return up;
}









rapidxml::xml_node<>* PathParameter::appendToNode
(
  const std::string& name,
  rapidxml::xml_document<>& doc,
  rapidxml::xml_node<>& node,
  boost::filesystem::path inputfilepath
) const
{
    insight::CurrentExceptionContext ex(3, "appending path "+name+" to node "+node.name());
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);

    FileContainer::appendToNode(
          doc, *child,
          inputfilepath
    );

    return child;

}

void PathParameter::readFromNode
(
  const std::string& name,
  rapidxml::xml_node<>& node,
  boost::filesystem::path inputfilepath
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    FileContainer::readFromNode(*child, inputfilepath);
    triggerValueChanged();
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % originalFilePath_.string()
           )
        );
  }
}

std::unique_ptr<PathParameter> PathParameter::clonePathParameter() const
{
  return std::make_unique<PathParameter>(
        originalFilePath_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order(),
        file_content_);
}

std::unique_ptr<Parameter> PathParameter::clone() const
{
  return clonePathParameter();
}

void PathParameter::copyFrom(const Parameter& p)
{
  operator=(dynamic_cast<const PathParameter&>(p));

}


void PathParameter::operator=(const PathParameter& op)
{
  FileContainer::operator=(op);
  Parameter::copyFrom(op);
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
        FileContainer(originalFilePath, tf),
        "temporary file path" );
}






defineType(DirectoryParameter);
addToFactoryTable(Parameter, DirectoryParameter);

DirectoryParameter::DirectoryParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(".", description, isHidden, isExpert, isNecessary, order)
{}

DirectoryParameter::DirectoryParameter(const boost::filesystem::path& value, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(value, description, isHidden, isExpert, isNecessary, order)
{}

std::string DirectoryParameter::latexRepresentation() const
{
    return std::string()
      + "{\\ttfamily "
      + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(originalFilePath_)) ).toLaTeX()
      + "}";
}

//std::string DirectoryParameter::plainTextRepresentation(int indent) const
//{
//    return std::string(indent, ' ')
//      + "\""
//      + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(value_)) ).toPlainText()
//      + "\"\n";
//}


rapidxml::xml_node<>* DirectoryParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value",
      doc.allocate_string(originalFilePath_.string().c_str())
    ));
    return child;
}

void DirectoryParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    originalFilePath_=boost::filesystem::path(child->first_attribute("value")->value());
    triggerValueChanged();
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % originalFilePath_.string()
           )
        );
  }
}


void DirectoryParameter::operator=(const DirectoryParameter &p)
{
  PathParameter::copyFrom( p );
}



std::unique_ptr<Parameter> DirectoryParameter::clone() const
{
  return cloneDirectoryParameter();
}

std::unique_ptr<DirectoryParameter> DirectoryParameter::cloneDirectoryParameter() const
{
  return std::make_unique<DirectoryParameter>(
        originalFilePath_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
}




}
