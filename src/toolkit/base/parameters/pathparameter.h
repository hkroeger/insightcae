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

#include <limits>
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"



namespace insight
{



/**
 * @brief The PathParameter class stores a reference to some external file.
 * It can store only the path but the external content can be "packed", i.e.
 * loaded and stored in memory. When the parameter is saved, the packed contents
 * will be stored in the parameter file in Base64 encoding.
 */
class PathParameter
    : public Parameter,
      public FileContainer
{

protected:
    void signalContentChange() override;

public:
  declareType ( "path" );

  PathParameter (
      const std::string& description,
      bool isHidden=false,
      bool isExpert=false,
      bool isNecessary=false,
      int order=0  );

  PathParameter (
      const boost::filesystem::path& value,
      const std::string& description,
      bool isHidden=false,
      bool isExpert=false,
      bool isNecessary=false,
      int order=0,
      std::shared_ptr<std::string> binary_content = std::shared_ptr<std::string>()  );

  PathParameter (
      const FileContainer& fc,
      const std::string& description,
      bool isHidden=false,
      bool isExpert=false,
      bool isNecessary=false,
      int order=0 );

  bool isDifferent(const Parameter& p) const override;

  std::string latexRepresentation() const override;
  std::string plainTextRepresentation(int /*indent*/=0) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;

  /**
   * @brief filePath
   * Get the path of the file.
   * It will be created, if it does not exist on the filesystem yet
   * but its content is available in memory.
   * @param baseDirectory
   * The working directory. If the file is only in memory,
   * it will be created in a temporary directory under this path.
   * @return
   */
  boost::filesystem::path filePath(boost::filesystem::path baseDirectory = "") const;

  rapidxml::xml_node<>* appendToNode(
      const std::string& name,
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath) const override;

  void readFromNode(
      const std::string& name,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath) override;

  PathParameter* clonePathParameter() const;
  Parameter* clone() const override;

  void copyFrom(const Parameter& p) override;
  void operator=(const PathParameter& p);

  int nChildren() const override;

};




std::shared_ptr<PathParameter> make_filepath(const boost::filesystem::path& path);

std::shared_ptr<PathParameter> make_filepath(const FileContainer& fc);

/**
 * @brief make_filepath
 * insert a vtkPolyData mesh as STL file
 * @param pd
 * @param originalFilePath
 * @return
 */
std::shared_ptr<PathParameter> make_filepath(
    vtkSmartPointer<vtkPolyData> pd,
    const boost::filesystem::path& originalFilePath );








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
    void readFromNode ( const std::string& name, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath ) override;

    void operator=(const DirectoryParameter& p);

    Parameter* clone() const override;
    DirectoryParameter* cloneDirectoryParameter() const;
};


}


#endif // PATHPARAMETER_H
