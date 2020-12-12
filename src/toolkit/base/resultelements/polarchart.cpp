#include "polarchart.h"

#include "gnuplot-iostream.h"

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

PolarChartRenderer::PolarChartRenderer(const ChartData *data, double phi_unit)
  : ChartRenderer(data),
    phi_unit_(phi_unit)
{}




void GnuplotPolarChartRenderer::gnuplotCommand(gnuplotio::Gnuplot& gp) const
{
 gp<<chartData_->addinit_<<";";
 gp<<"unset border;"
     " set polar;"
     " set grid polar 60.*pi/180.;"
     " set trange [0:2.*pi];"
     " set key rmargin;"
     " set size square;"
     " unset xtics;"
     " unset ytics;"
     ;

 double rmax=0.;
 for ( const PlotCurve& pc: chartData_->plc_ ) {
  rmax=std::max(rmax, pc.xy().col(1).max());
 }

 gp<<"set_label(x, text) = sprintf(\"set label '%s' at ("<<rmax<<"*1.05*cos(%f)), ("<<rmax<<"*1.05*sin(%f)) center\", text, x, x);"
  <<"eval set_label(0, \"$0^\\\\circ$\");"
 <<"eval set_label(60.*pi/180., \"$60^\\\\circ$\");"
 <<"eval set_label(120.*pi/180., \"$120^\\\\circ$\");"
 <<"eval set_label(180.*pi/180., \"$180^\\\\circ$\");"
 <<"eval set_label(240.*pi/180., \"$240^\\\\circ$\");"
 <<"eval set_label(300.*pi/180., \"$300^\\\\circ$\");";

 //gp<<"set xlabel '"<<xlabel_<<"'; set ylabel '"<<ylabel_<<"'; ";

 if ( chartData_->plc_.size() >0 )
 {
  gp<<"plot ";
  bool is_first=true;

  for ( const PlotCurve& pc: chartData_->plc_ )
  {
   if ( !pc.plotcmd_.empty() )
   {

    if (!is_first)
    {
     gp << ",";
    }
    else is_first=false;

    if ( pc.xy_.n_rows>0 )
    {
     gp<<"'-' "<<pc.plotcmd_;
    }
    else
    {
     gp<<pc.plotcmd_;
    }

   }
  }

  gp<<endl;

  for ( const PlotCurve& pc: chartData_->plc_ )
  {
   if ( pc.xy_.n_rows>0 )
   {
    arma::mat xy = pc.xy_;
    xy.col(0) *= phi_unit_;
    gp.send1d ( xy );
   }
  }

 }
}

GnuplotPolarChartRenderer::GnuplotPolarChartRenderer(const ChartData *data, double phi_unit)
  : GnuplotRendererBase<PolarChartRenderer>(data, phi_unit)
{}



} // namespace insight
