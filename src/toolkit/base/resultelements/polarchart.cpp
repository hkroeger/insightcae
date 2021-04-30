#include "polarchart.h"

#include "base/resultelements/gnuplotpolarchartrenderer.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;



namespace insight {




defineType(PolarChart);
addToFactoryTable(ResultElement, PolarChart);



PolarChart::PolarChart(const std::string& shortdesc, const std::string& longdesc, const std::string& unit)
: Chart(shortdesc, longdesc, unit)
{}



PolarChart::PolarChart
(
  const std::string& rlabel,
  const PlotCurveList& plc,
  const std::string& shortDesc, const std::string& longDesc,
  double phi_unit,
  const std::string& addinit
)
: Chart("", rlabel, plc, shortDesc, longDesc, addinit),
  phi_unit_(phi_unit)
{}




void PolarChart::generatePlotImage(const path &imagepath) const
{
  std::shared_ptr<PolarChartRenderer> renderer( new GnuplotPolarChartRenderer(chartData(), phi_unit_) );

  renderer->render(imagepath);
}





ResultElementPtr PolarChart::clone() const
{
    ResultElementPtr res ( new PolarChart ( ylabel_, plc_, shortDescription().simpleLatex(), longDescription().simpleLatex(), phi_unit_, addinit_ ) );
    res->setOrder ( order() );
    return res;
}







insight::ResultElement& addPolarPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& ,
    const std::string& resultelementname,
    const std::string& rlabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    double phi_unit,
    const std::string& addinit,
    const std::string& watermarktext
)
{
    std::string precmd=addinit+";";
    if ( watermarktext!="" ) {
        precmd+=
          "set label "
          "'"+SimpleLatex( watermarktext ).toLaTeX()+"'"
          " center at screen 0.5, 0.5 tc rgb\"#cccccc\" rotate by 30 font \",24\";"
          ;
    }

    return results->insert ( resultelementname,
                             new PolarChart
                             (
                                 rlabel, plc,
                                 shortDescription, "",
                                 phi_unit,
                                 precmd
                               ) );
}









} // namespace insight
