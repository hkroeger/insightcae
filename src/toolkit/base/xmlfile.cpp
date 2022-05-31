#include "xmlfile.h"

#include "base/exception.h"
#include "base/tools.h"

#include "rapidxml/rapidxml_print.hpp"

using namespace rapidxml;

namespace insight {

XMLFile::XMLFile()
{
    xml_node<>* decl = this->allocate_node(node_declaration);
    decl->append_attribute(this->allocate_attribute("version", "1.0"));
    decl->append_attribute(this->allocate_attribute("encoding", "utf-8"));
    this->append_node(decl);
    rootNode_ = this->allocate_node(node_element, "root");
    this->append_node(rootNode_);
}

XMLFile::XMLFile(const boost::filesystem::path &fileName)
{
    CurrentExceptionContext ex("reading xml document from file "+fileName.string());

    std::string contents;
    readFileIntoString(fileName, contents);

    this->parse<0>(&contents[0]);

    rootNode_ = this->first_node("root");
}

void XMLFile::saveToFile(const boost::filesystem::path &file) const
{
    CurrentExceptionContext ex("writing xml document to file "+file.string());
    std::ofstream f(file.string());
    f << (*this) << std::endl << std::flush;
    f.close();
}

} // namespace insight
