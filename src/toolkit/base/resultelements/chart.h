#ifndef INSIGHT_CHART_H
#define INSIGHT_CHART_H


#include "base/resultelementcollection.h"



namespace insight {


struct PlotCurveStyle
{
#define ADD(NAME, SETFNAME, TYPE, DEF) \
  TYPE NAME##_=DEF; \
  PlotCurveStyle& set_##NAME(const TYPE& v) { NAME##_=v; return *this; } \
  PlotCurveStyle& SETFNAME(const TYPE& v) { NAME##_=v; return *this; }

  ADD(color, lc, int, -1)
  ADD(lineWidth, lw, int, 1)
  ADD(dashType, dt, int, 0)

  ADD(withPoints, wp, bool, true)
  ADD(withLines, wl, bool, false)

  PlotCurveStyle& wlp() { set_withPoints(true); set_withLines(true); return *this; }

  ADD(errorLines, el, bool, false)
  ADD(title, t, std::string, "") // automatic
  ADD(ax_y, y, int, 1)

  PlotCurveStyle& no_t() { title_=""; return *this; }

#undef ADD
};


struct PlotCurve {
    arma::mat xy_;
    std::string plotcmd_;
    PlotCurveStyle style_;

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

    PlotCurve ( const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const PlotCurveStyle& style );

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





struct ChartData
{
  std::string xlabel_;
  std::string ylabel_;
  PlotCurveList plc_;
  std::string addinit_;
};




class Chart
    : public ResultElement,
      protected ChartData
{
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

    const ChartData* chartData() const;
    void addCurve(const PlotCurve& pc);
    const PlotCurve& plotCurve(const std::string& plainTextLabel) const;
    virtual void generatePlotImage ( const boost::filesystem::path& imagepath ) const;

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


insight::ResultElement& addPlot
(
    ResultElementCollection& results,
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


#ifdef SWIG
%template(vector_PlotCurve) std::vector<insight::PlotCurve>;
#endif

#endif // INSIGHT_CHART_H
