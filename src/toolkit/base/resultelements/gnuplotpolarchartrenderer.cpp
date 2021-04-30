#include "gnuplotpolarchartrenderer.h"

#include "base/resultelements/chart.h"

namespace insight {


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
