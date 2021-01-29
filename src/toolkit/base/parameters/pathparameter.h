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

#ifndef PATHPARAMETER_H
#define PATHPARAMETER_H

#include "base/filecontainer.h"
#include "base/parameter.h"





namespace insight
{




class PathParameter
    : public Parameter,
      public FileContainer
{


public:
    declareType ( "path" );

    PathParameter ( const std::string& description,
                    bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
        );

    PathParameter ( const boost::filesystem::path& value, const std::string& description,
                    bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0,
                    std::shared_ptr<std::string> binary_content = std::shared_ptr<std::string>()
        );

    PathParameter ( const FileContainer& fc, const std::string& description,
                    bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
        );

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int /*indent*/=0) const override;

    bool isPacked() const override;
    void pack() override;
    void unpack(const boost::filesystem::path& basePath) override;
    void clearPackedData() override;

    rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
     boost::filesystem::path inputfilepath) const override;

    void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
     boost::filesystem::path inputfilepath) override;

    PathParameter* clonePathParameter() const;
    Parameter* clone() const override;
    void reset(const Parameter& p) override;

    void operator=(const PathParameter& op);
    void operator=(const FileContainer& oc);

};


std::shared_ptr<PathParameter> make_filepath(const boost::filesystem::path& path);
std::shared_ptr<PathParameter> make_filepath(const FileContainer& fc);








//template<> rapidxml::xml_node<>* SimpleParameter<boost::filesystem::path, PathName>::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
//    boost::filesystem::path inputfilepath) const;

//template<> void SimpleParameter<boost::filesystem::path, PathName>::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
//  boost::filesystem::path inputfilepath);




class DirectoryParameter
    : public PathParameter
{
public:
    declareType ( "directory" );

    DirectoryParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    DirectoryParameter ( const boost::filesystem::path& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    std::string latexRepresentation() const override;

    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override;
    void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath ) override;

    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};


}


#endif // PATHPARAMETER_H
