#ifndef INSIGHT_CONTOURCHART_H
#define INSIGHT_CONTOURCHART_H


#include "base/resultelements/chart.h"
#include "base/resultelements/chartrenderer.h"
#include "base/resultelements/latexgnuplotrenderer.h"
#include "gnuplot-iostream.h"


namespace insight {



struct PlotField
{
    arma::mat x_, y_;

    /**
     * @brief data_
     * has as many rows as x has elements and as many cols as y elements
     */
    arma::mat data_;

    PlotField();
    PlotField ( const arma::mat& x,
                const arma::mat& y,
                const arma::mat& data  );
};


class PolarContourChart;

class GnuplotPolarContourChartRenderer
        : public LaTeXGnuplotRenderer<ChartRenderer>
{
    const PolarContourChart& pcc_;
    insight::TemporaryFile df_;

protected:
    double canvasSizeRatio() const override;

public:
    GnuplotPolarContourChartRenderer(const PolarContourChart& pcc);

    virtual void gnuplotCommand(gnuplotio::Gnuplot&) const;
};


class PolarContourChart
    : public ResultElement,
      public PlotField
{
    friend class GnuplotPolarContourChartRenderer;

    std::string rlabel_, cblabel_;
    double rmax_;
    std::vector<double> contourLines_;
    boost::optional<double> zClipMin_;
    boost::optional<double> zClipMax_;


public:
    declareType ( "PolarContourChart" );

    PolarContourChart(
            const std::string& shortdesc,
            const std::string& longdesc,
            const std::string& unit
            );

    PolarContourChart
    (
        const std::string& rlabel,
        const std::string& cblabel,
        double rmax,
        const PlotField& plf,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::vector<double>& contourLines,
        boost::optional<double> zClipMin = boost::optional<double>(),
        boost::optional<double> zClipMax = boost::optional<double>()
    );


    virtual void generatePlotImage ( const boost::filesystem::path& imagepath ) const;

    void insertLatexHeaderCode ( std::set<std::string>& hc ) const override;
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;
    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

    rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const override;

    void readFromNode
        (
            const std::string& name,
            const rapidxml::xml_node<>& node
        ) override;

    ResultElementPtr clone() const override;
};


void addPolarContourPlot
(
        std::shared_ptr<ResultElementCollection> results,
        const std::string& resultelementname,
        const std::string& rlabel,
        const std::string& cblabel,
        double rmax,
        const PlotField& plf,
        const std::string& shortDescription,
        const std::vector<double>& contourLines,
        boost::optional<double> zClipMin = boost::optional<double>(),
        boost::optional<double> zClipMax = boost::optional<double>()
);



} // namespace insight

#endif // INSIGHT_CONTOURCHART_H
