#ifndef INSIGHT_TABULARRESULT_H
#define INSIGHT_TABULARRESULT_H


#include "base/resultelement.h"


namespace insight {



class TabularResult
    : public ResultElement
{
public:
    typedef std::vector<double> Row;
    typedef std::vector<Row> Table;

protected:
    std::vector<std::string> headings_;
    Table rows_;

public:
    declareType ( "TabularResult" );

    TabularResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );

    TabularResult
    (
        const std::vector<std::string>& headings,
        const Table& rows,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit
    );

    TabularResult
    (
        const std::vector<std::string>& headings,
        const arma::mat& rows,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit
    );

    inline const std::vector<std::string>& headings() const
    {
        return headings_;
    }
    inline const Table& rows() const
    {
        return rows_;
    }
    inline Row& appendRow()
    {
        rows_.push_back ( Row ( headings_.size() ) );
        return rows_.back();
    }
    void setCellByName ( Row& r, const std::string& colname, double value );

    arma::mat getColByName ( const std::string& colname ) const;

    arma::mat toMat() const;

    inline void setTableData ( const std::vector<std::string>& headings, const Table& rows )
    {
        headings_=headings;
        rows_=rows;
    }

    virtual void writeGnuplotData ( std::ostream& f ) const;
    void insertLatexHeaderCode ( std::set<std::string>& f ) const override;
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;
    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

    /**
     * append the contents of this element to the given xml node
     */
    rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const override;

    void readFromNode
        (
            const std::string& name,
            rapidxml::xml_node<>& node
        ) override;

    ResultElementPtr clone() const override;
};



} // namespace insight

#endif // INSIGHT_TABULARRESULT_H
