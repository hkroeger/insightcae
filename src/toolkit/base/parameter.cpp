/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "parameter.h"
#include "base/exception.h"
#include "base/factory.h"
#include "base/latextools.h"
#include "base/tools.h"
#include <iostream>

#include <iterator>
#include <memory>
#include "boost/algorithm/string/trim.hpp"

#include "base/parameterset.h"
#include "base/parameters/selectablesubsetparameter.h"
#include "base/rapidxml.h"
#include "base/cppextensions.h"

#include "boost/date_time/gregorian/parsers.hpp"
#include "boost/date_time/posix_time/time_formatters.hpp"
#include "boost/filesystem/operations.hpp"
#include "rapidxml/rapidxml_print.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;



namespace insight 
{




defineType(Parameter);
// defineFactoryTable(Parameter, LIST(const std::string& desc), LIST(desc) );
defineFactoryTable2(Parameter, ParameterFromDescription, createParameter);
defineFactoryTable2(Parameter, ParameterFromNode, createParameterFromNode);


void Parameter::setParent(Element *parent)
{
    Element::setParent(parent);
}


Parameter::Parameter(const rapidxml::xml_node<> &node)
  : Element( getOptionalAttributeOrDefault<int>(node, "order", 0) ),
    description_(getOptionalAttributeOrDefault(node, "description", std::string())),
    isHidden_(getOptionalAttributeOrDefault<bool>(node, "isHidden", false)),
    isExpert_(getOptionalAttributeOrDefault<bool>(node, "isExpert", false)),
    isNecessary_(getOptionalAttributeOrDefault<bool>(node, "isNecessary", false))
{}




Parameter::Parameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary,
    int order )
: Element(order),
  description_(description),
  isHidden_(isHidden), isExpert_(isExpert), isNecessary_(isNecessary)
{
}




bool Parameter::hasParentSet() const
{
    return hasParent()
           && dynamic_cast<const ParameterSet*>(&parent());
}





ParameterSet& Parameter::parentSet()
{
    return dynamic_cast<ParameterSet&>(parent());
}



bool Parameter::isHidden() const { return isHidden_; }
bool Parameter::isExpert() const {return isExpert_; }
bool Parameter::isNecessary() const { return isNecessary_; }

bool Parameter::isDifferent(const Parameter &) const
{
  throw insight::Exception("cannot compare parameters: the isDifferent function is not implemented1");
  return true;
}



bool Parameter::isModified(const ParameterSet &defaultValues) const
{
    bool cmodified = false;
    try
    {
        if (nChildren()>0) // has children
        {
            for (int k=0; k<nChildren(); ++k)
            {
                if (auto *cp=dynamic_cast<const Parameter*>(&childElement(k)))
                {
                    cmodified |= cp->isModified(defaultValues);
                }
            }
        }
        else
        {
            auto pp=path(true);
            if (defaultValues.hasPath(pp))
            {
                const auto& dp = defaultValues.get<insight::Parameter>(pp);
                cmodified = isDifferent(dp);
            }
        }
    }
    catch (...)
    {
        cmodified=true;
    }

    return cmodified;
}

void Parameter::resolveRelativePaths(const boost::filesystem::path &baseDirectory)
{
    for (size_t i=0; i<nChildren(); ++i)
    {
        if (auto *cp = dynamic_cast<Parameter*>(&childElementRef(i)))
        {
            cp->resolveRelativePaths(baseDirectory);
        }
    }
}


// rapidxml::xml_node<>* Parameter::appendToNode(
//         const std::string& name,
//         rapidxml::xml_document<>& doc,
//         rapidxml::xml_node<>& node,
//         boost::filesystem::path ) const
// {
//   using namespace rapidxml;

//   if (name.empty())
//   {
//         return &node;
//   }
//   else
//   {
//         xml_node<>* child = doc.allocate_node(node_element, doc.allocate_string(this->type().c_str()));
//         node.append_node(child);
//         child->append_attribute(
//             doc.allocate_attribute
//             (
//                 "name",
//                 doc.allocate_string(name.c_str()))
//             );

//         return child;
//   }
// }



// void Parameter::saveToNode(
//     xml_document<>& doc,
//     xml_node<>& rootNode,
//     const boost::filesystem::path& parent_path,
//     std::string analysisName ) const
// {
//     CurrentExceptionContext ex(
//         Loops,
//         "writing parameter %s content into XML node"
//         " (parent path %s, analysis name %s",
//         type().c_str(), parent_path.string().c_str(), analysisName.c_str());

//     // insert analysis name
//     if (analysisName != "")
//     {
//         xml_node<> *analysisnamenode = doc.allocate_node(node_element, "analysis");
//         rootNode.append_node(analysisnamenode);
//         analysisnamenode->append_attribute(
//             doc.allocate_attribute
//             (
//                 "name",
//                 doc.allocate_string(analysisName.c_str())
//                 ));
//     }

//     // store parameters
//     appendToNode(std::string(), doc, rootNode, parent_path);
// }




// void Parameter::saveToStream(
//     std::ostream& os,
//     const boost::filesystem::path& parent_path,
//     std::string analysisName ) const
// {
//   CurrentExceptionContext ex(Loops,
//       "writing parameter %s content into output stream (parent path %s, analysis name %s",
//       type().c_str(), parent_path.string().c_str(), analysisName.c_str());

//   // prepare XML document
//   xml_document<> doc;
//   xml_node<>* decl = doc.allocate_node(node_declaration);
//   decl->append_attribute(doc.allocate_attribute("version", "1.0"));
//   decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
//   doc.append_node(decl);
//   xml_node<> *rootNode = doc.allocate_node(node_element, "root");
//   doc.append_node(rootNode);

//   saveToNode(doc, *rootNode, parent_path, analysisName);

//   os << doc;
// }

// void Parameter::saveToFile(const boost::filesystem::path& file, std::string analysisName ) const
// {
//   CurrentExceptionContext ex(insight::VerbosityLevel::BasicBusiness, "writing parameter set to file "+file.string());
//   std::ofstream f(file.c_str());
//   saveToStream( f, file.parent_path(), analysisName );
//   f << std::endl;
//   f << std::flush;
//   f.close();
// }


// void Parameter::saveToString(std::string &s, const boost::filesystem::path& file, std::string analysisType) const
// {
//   std::ostringstream os(s, std::ios_base::ate);
//   saveToStream(os, file.parent_path(), analysisType);
//   s = os.str();
// }



// std::string Parameter::readFromRootNode(
//     const xml_node<>& rootNode,
//     const boost::filesystem::path& parent_path,
//     const std::string& startAtSubnode )
// {
//     CurrentExceptionContext ex("reading parameter %s from XML node", type().c_str());

//     std::string analysisName;
//     auto *analysisnamenode = rootNode.first_node("analysis");
//     if (analysisnamenode)
//     {
//         analysisName = analysisnamenode->first_attribute("name")->value();
//     }

//     auto* crn=&rootNode;
//     if (!startAtSubnode.empty())
//     {
//         std::vector<std::string> path;
//         boost::split(path, startAtSubnode, boost::is_any_of("/"));
//         for (const auto& p: path)
//         {
//             std::map<std::string, xml_node<>*> nodes;
//             for (auto *e = crn->first_node(); e!=nullptr; e=e->next_sibling())
//             {
//                 nodes[ e->first_attribute("name")->value() ]=e;
//             }

//             auto e = nodes.find(p);
//             if (e==nodes.end())
//             {
//                 std::ostringstream os;
//                 for(auto& n: nodes) os<<" "<<n.first;
//                 throw insight::Exception(
//                     "Could not find node "+p+" (full path "+startAtSubnode+")!\n"
//                                                                                    "Available:"+os.str());
//             }
//             else
//             {
//                 crn=e->second;
//             }
//         }
//     }

//     readFromNode(std::string(), *crn, parent_path);

//     return analysisName;
// }




// string Parameter::readFromFile(
//     const boost::filesystem::path &file,
//     const std::string &startAtSubnode )
// {
//     CurrentExceptionContext ex(
//         "reading parameter %s from file %s",
//         type().c_str(), file.string().c_str() );

//     std::string contents;
//     readFileIntoString(file, contents);

//     xml_document<> doc;
//     doc.parse<0>(&contents[0]);

//     xml_node<> *rootnode = doc.first_node("root");

//     return readFromRootNode(*rootnode, file.parent_path(), startAtSubnode);
// }




bool Parameter::isPacked() const
{
  return false;
}

void Parameter::pack()
{
    // do nothing by default
}

void Parameter::unpack(const boost::filesystem::path&)
{
    // do nothing by default
}

void Parameter::clearPackedData()
{
    // do nothing by default
}

void Parameter::readFromRootNode(
    const rapidxml::xml_node<> &rootNode,
    const std::string &startAtSubnode)
{
    hierarchicalData::Element::readFromRootNode(
        rootNode, startAtSubnode);
}

void Parameter::readFromFile(
    const boost::filesystem::path& file,
    const std::string& startAtSubnode )
{
    Element::readFromFile(file, startAtSubnode);
    resolveRelativePaths(file.parent_path());
}

rapidxml::xml_node<> *Parameter::appendToNode(
    const std::string &name,
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node,
    const OutputProperties& outProps) const
{
    auto c=Element::appendToNode(name, doc, node, outProps);
    if (!outProps.skipParameterDescription)
    {
        appendAttribute(doc, *c, "description", description_.simpleLatex());
    }
    return c;
}




std::unique_ptr<Parameter> Parameter::createFromNode(
    const rapidxml::xml_node<> &node )
{
    std::string ptype = node.name();
    return createParameterFromNode().lookup(ptype)(node);
}




void Parameter::assignFrom(const Element &o)
{
  auto& op=dynamic_cast<const Parameter&>(o);

  description_ = op.description_;
  isHidden_ = op.isHidden_;
  isExpert_ = op.isExpert_;
  isNecessary_ = op.isNecessary_;

  Element::assignFrom(o);
}





std::unique_ptr<Parameter>
Parameter::intersection(const Parameter &other) const
{
  return cloneAs<Parameter>();
}








}
