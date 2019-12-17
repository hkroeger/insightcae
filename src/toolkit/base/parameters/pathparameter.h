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

#include "base/parameter.h"




namespace boost
{
namespace filesystem
{

template < >
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt);

//boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To );

}
}






namespace insight
{




std::string base64_encode(const std::string& s);
std::string base64_encode(const boost::filesystem::path& f);
std::string base64_decode(const std::string& s);






class PathParameter
    : public Parameter
{

protected:
  /**
     * @brief value_
     * original file name
     */
  boost::filesystem::path value_;

  /**
   * @brief file_content_
   * Store content of file, if packed.
   * Contains plain file content, not encoded.
   */
  std::shared_ptr<std::string> file_content_;
  unsigned char file_content_hash_[MD5_DIGEST_LENGTH];

  mutable std::unique_ptr<std::istream> file_content_stream_;

  boost::filesystem::path unpackFilePath(boost::filesystem::path baseDirectory = "") const;

public:
    declareType ( "path" );

    PathParameter ( const std::string& description,
                    bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0
        );

    PathParameter ( const boost::filesystem::path& value, const std::string& description,
                    bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0,
                    std::shared_ptr<std::string> base64_content = std::shared_ptr<std::string>()
        );


    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int /*indent*/=0) const override;

    bool isValid() const;

    /**
     * @brief filePath
     * Get the path of the file.
     * It will be created, if it does not exist on the filesystem yet.
     * @param baseDirectory
     * The working directory. If the file is only in memory,
     * it will be created in a temporary directory under this path.
     * @return
     */
    boost::filesystem::path filePath(boost::filesystem::path baseDirectory = "") const;

    /**
     * @brief originalFilePath
     * The path of the originally referenced file.
     * Since the file might have been packed into the parameter set,
     * this file must not necessarily be present under the specified path.
     * @return
     */
    const boost::filesystem::path& originalFilePath() const;

    /**
     * @brief fileName
     * @return
     * returns the file name component only
     */
    boost::filesystem::path fileName() const;

    void setOriginalFilePath(const boost::filesystem::path& value);


    std::istream& stream() const;


    /**
     * @brief isPacked
     * check, if contains file contents
     * @return
     */
    bool isPacked() const override;

    /**
     * @brief pack
     * pack the external file. Replace stored content, if present.
     */
    void pack() override;

    /**
     * @brief unpack
     * restore file contents on disk, if file is not there
     */
    void unpack(const boost::filesystem::path& basePath) override;

    /**
     * @brief copyTo
     * Unpack or copy file to given location
     * @param filePath
     */
    void copyTo(const boost::filesystem::path& filePath) const;

    /**
     * @brief replaceContent
     * reads the content from specified file. The original file path is retained.
     * @param filePath
     */
    void replaceContent(const boost::filesystem::path& filePath);

    void clearPackedData() override;


    rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
     boost::filesystem::path inputfilepath) const override;

    void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
     boost::filesystem::path inputfilepath) override;

    PathParameter* clonePathParameter() const;
    Parameter* clone() const override;
    void reset(const Parameter& p) override;

    void operator=(const PathParameter& op);

};


std::shared_ptr<PathParameter> make_filepath(const boost::filesystem::path& path);








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
