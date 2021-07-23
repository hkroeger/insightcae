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
#include "base/resultelements/attributeresulttable.h"
#include "base/resultelements/tabularresult.h"
#include "base/resultelements/chart.h"
#include "base/resultelements/polarchart.h"
#include "base/resultelements/contourchart.h"

namespace insight 
{



class ResultSet
    :
    public ResultElementCollection,
    public ResultElement
{
protected:
    ParameterSet p_;
    std::string title_, subtitle_, date_, author_;
    std::string introduction_;

public:
    declareType ( "ResultSet" );

    ResultSet( const boost::filesystem::path& fileName, const std::string& analysisName = "" );

    /**
     * @brief ResultSet
     * stream needs to opened in binary!! (mode std::ios::in | std::ios::binary)
     * @param is
     * @param analysisName
     */
    ResultSet( std::istream& is, const std::string& analysisName = "" );

    ResultSet( std::string& cont, const std::string& analysisName = "" );

    ResultSet
    (
        const ParameterSet& p,
        const std::string& title,
        const std::string& subtitle,
        const std::string *author = NULL,
        const std::string *date = NULL
    );


    /* =======================================================================================================*/
    /* =======================================================================================================*/
    /* =======================================================================================================*/
    //   Required, because ResultSet is inserted as subsection into some results,
    // consider removing but will break backwards compat..
    ResultSet ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit="" );
    /**
     * restore the result elements from the given node
     */
    void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node ) override;
    /* =======================================================================================================*/
    /* =======================================================================================================*/
    /* =======================================================================================================*/

    ResultSet ( const ResultSet& other );
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

    void transfer ( const ResultSet& other );
    inline const ParameterSet& parameters() const
    {
        return p_;
    }

    void writeLatexHeaderCode ( std::ostream& f ) const override;
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;

    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

    virtual void writeLatexFile ( const boost::filesystem::path& file ) const;
    virtual void generatePDF ( const boost::filesystem::path& file ) const;

    /**
     * append the contents of this element to the given xml node
     */
    rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const override;


    /**
     * save result set to XML file
     */
    virtual void saveToFile ( const boost::filesystem::path& file ) const;
    virtual void saveToStream( std::ostream& os ) const;

    /**
     * read result set from xml file
     */
    virtual void readFrom ( const boost::filesystem::path& file );
    virtual void readFrom ( std::istream& is );
    virtual void readFrom ( std::string& contents );

    virtual ParameterSetPtr convertIntoParameterSet() const;
    ParameterPtr convertIntoParameter() const override;

    ResultElementPtr clone() const override;
};


typedef std::shared_ptr<ResultSet> ResultSetPtr;



}




#endif // INSIGHT_RESULTSET_H
