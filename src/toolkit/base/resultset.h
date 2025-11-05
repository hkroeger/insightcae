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


#ifndef INSIGHT_RESULTSET_H
#define INSIGHT_RESULTSET_H

#include "base/units.h"
#include "base/parameterset.h"


#include "base/resultelementcollection.h"
#include "base/resultelements/resultsection.h"

#include "base/resultelements/comment.h"
#include "base/resultelements/numericalresult.h"
#include "base/resultelements/scalarresult.h"
#include "base/resultelements/vectorresult.h"
#include "base/resultelements/image.h"
#include "base/resultelements/video.h"
#include "base/resultelements/attributeresulttable.h"
#include "base/resultelements/tabularresult.h"
#include "base/resultelements/chart.h"
#include "base/resultelements/polarchart.h"
#include "base/resultelements/contourchart.h"
#include "boost/filesystem/path.hpp"

namespace insight 
{

class ResultSet;

typedef std::shared_ptr<ResultSet> ResultSetPtr;


class ResultSet
: public ResultElementCollection
{
protected:
    std::unique_ptr<ParameterSet> p_;
    std::string title_, subtitle_, date_, author_;
    std::string introduction_;

public:
    declareType ( "ResultSet" );

    ResultSet
    (
        std::unique_ptr<ParameterSet> p = nullptr,
        const std::string& title = std::string(),
        const std::string& subtitle = std::string(),
        const std::string *author = NULL,
        const std::string *date = NULL
    );

    static ResultSetPtr createFromFile(
        const boost::filesystem::path& fileName,
        std::unique_ptr<ParameterSet> p = nullptr );

    /**
     * stream needs to opened in binary!! (mode std::ios::in | std::ios::binary)
     */
    static ResultSetPtr createFromStream(
        std::istream& is,
        std::unique_ptr<ParameterSet> p = nullptr );

    static ResultSetPtr createFromString(
        const std::string& cont,
        std::unique_ptr<ParameterSet> p = nullptr );


    /* =======================================================================================================*/
    /* =======================================================================================================*/
    /* =======================================================================================================*/
    //   Required, because ResultSet is inserted as subsection into some results,
    // consider removing but will break backwards compat..
    // ResultSet ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit="" );

    /* =======================================================================================================*/
    /* =======================================================================================================*/
    /* =======================================================================================================*/

    // ResultSet ( const ResultSet& other );
    virtual ~ResultSet();

    inline std::string& introduction()
    {
        return introduction_;
    }

    inline const std::string& title() const
    {
        return title_;
    }
    inline const std::string& subtitle() const
    {
        return subtitle_;
    }

    inline const std::string author() const { return author_; }
    inline const std::string date() const { return date_; }

    void transfer ( const ResultSet& other );

    inline const ParameterSet& parameters() const
    {
        return *p_;
    }
    void clearInputParameters();

    void insertLatexHeaderCode ( std::set<std::string>& hc ) const override;
    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    static boost::filesystem::path
    reportDataPath(const boost::filesystem::path& outFileName);

    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

    virtual void writeLatexFile ( const boost::filesystem::path& file ) const;
    virtual void generatePDF ( const boost::filesystem::path& file ) const;

    /**
     * append the result elements to the given xml node
     */
    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node) const override;

    /**
     * restore the result elements from the given node
     */
    const rapidxml::xml_node<>* readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node) override;

    void saveAs(const boost::filesystem::path &outfile) const;

    std::unique_ptr<ParameterSet> convertIntoParameterSet() const;
    std::unique_ptr<Parameter> convertIntoParameter() const override;

    std::unique_ptr<hierarchicalData::Element> clone() const override;
};



}




#endif // INSIGHT_RESULTSET_H
