#include "polarchart.h"

#include "base/resultelements/fastgnuplotrenderer.h"
#include "base/resultelements/gnuplotpolarchartrenderer.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
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




void PolarChart::generatePlotImage(const boost::filesystem::path &imagepath) const
{
  std::shared_ptr<PolarChartRenderer> renderer;

  std::string selectedRenderer="gnuplot";

#if !defined(CHART_RENDERER_GNUPLOT) && defined(CHART_RENDERER_MATPLOTLIB)
  selectedRenderer="matplotlib";
#endif

  if (const auto* rv=getenv("INSIGHT_CHARTRENDERER"))
    selectedRenderer=rv;

#ifdef CHART_RENDERER_GNUPLOT
  if (selectedRenderer=="gnuplot")
  {
    renderer = std::make_shared<GnuplotPolarChartRenderer<LaTeXGnuplotRenderer<PolarChartRenderer> > >
          (chartData(), phi_unit_);
  }
  else if (selectedRenderer=="fastgnuplot")
  {
      renderer = std::make_shared<GnuplotPolarChartRenderer<FastGnuplotRenderer<PolarChartRenderer> > >
            (chartData(), phi_unit_);
  }
#else
  throw insight::Exception("There is no polar chart renderer available!");
#endif
  renderer->render(imagepath);
}





std::unique_ptr<hierarchicalData::Element> PolarChart::cloneUninitialized() const
{
    auto res = std::make_unique<PolarChart> (
        ylabel_, plc_,
        shortDescription().simpleLatex(), longDescription().simpleLatex(),
        phi_unit_, addinit_ );
    res->setOrder ( order() );
    return res;
}

bool PolarChart::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const PolarChart*>(&op))
    {
        if (phi_unit_!=oa->phi_unit_)
            return false;
        return Chart::isEqual(*oa);
    }
    else
        return false;
}







insight::ResultElement& addPolarPlot
(
    ResultElementCollection& results,
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

    return results.insert<PolarChart>(
        resultelementname,
        rlabel, plc,
        shortDescription, "",
        phi_unit,
        precmd
      );
}









} // namespace insight
