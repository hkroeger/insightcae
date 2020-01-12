#ifndef INSIGHT_CHART_H
#define INSIGHT_CHART_H


#include "base/resultelementcollection.h"


namespace gnuplotio {
 class Gnuplot;
}



namespace insight {




struct PlotCurve {
    arma::mat xy_;
    std::string plotcmd_;

    /**
     * curve identifier on plain-text-level
     */
    std::string plaintextlabel_;

    PlotCurve();
    PlotCurve ( const PlotCurve& o );

    /**
     * construct a plot curve by a gnuplot command only, i.e. a formula
     */
    PlotCurve ( const std::string& plaintextlabel, const char* plotcmd );

    /**
     * construct from separate x and y arrays (sizes have to match)
     */
    PlotCurve ( const std::vector<double>& x, const std::vector<double>& y, const std::string& plaintextlabel, const std::string& plotcmd = "" );

    /**
     * construct from separate x and y column vectors (sizes have to match)
     */
    PlotCurve ( const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const std::string& plotcmd );

    /**
     * construct a horizontal line spanning xrange with value y
     */
    PlotCurve ( const arma::mat& xrange, double y, const std::string& plaintextlabel, const std::string& plotcmd );

    /**
     * construct from matrix containing two columns with x and y values
     */
    PlotCurve ( const arma::mat& xy, const std::string& plaintextlabel, const std::string& plotcmd = "w l" );

    /**
     * sort the curve values by x.
     * This needs to be explicitly called, because it is not always wanted (e.g. parametric plots)
     */
    void sort();

    std::string title() const;
    const arma::mat& xy() const;
    const std::string& plaintextlabel() const;
};




struct PlotCurveList
    : public std::vector<PlotCurve>
{
 PlotCurveList();
 PlotCurveList(size_t n);
 PlotCurveList(std::initializer_list<PlotCurve> il);
 PlotCurveList(std::vector<PlotCurve>::const_iterator begin, std::vector<PlotCurve>::const_iterator end);

 bool include_zero=true;
};


#ifdef SWIG
%template(vector_PlotCurve) std::vector<insight::PlotCurve>;
#endif


class Chart
    : public ResultElement
{
protected:
    std::string xlabel_;
    std::string ylabel_;
    PlotCurveList plc_;
    std::string addinit_;

public:
    declareType ( "Chart" );

    Chart ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Chart
    (
        const std::string& xlabel,
        const std::string& ylabel,
        const PlotCurveList& plc,
        const std::string& shortDesc, const std::string& longDesc,
        const std::string& addinit = ""
    );

    virtual void gnuplotCommand(gnuplotio::Gnuplot&) const;
    virtual void generatePlotImage ( const boost::filesystem::path& imagepath ) const;

    void writeLatexHeaderCode ( std::ostream& f ) const override;
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
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node
        ) override;

    ResultElementPtr clone() const override;
};




insight::ResultElement& addPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    const std::string& addinit = "",
    const std::string& watermarktext = ""
);




} // namespace insight

#endif // INSIGHT_CHART_H
