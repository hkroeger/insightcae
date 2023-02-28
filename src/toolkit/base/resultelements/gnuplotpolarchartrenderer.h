#ifndef INSIGHT_GNUPLOTPOLARCHARTRENDERER_H
#define INSIGHT_GNUPLOTPOLARCHARTRENDERER_H

#include "polarchartrenderer.h"
#include "base/resultelements/latexgnuplotrenderer.h"
#include "base/resultelements/chart.h"

namespace insight {

template<class Base>
class GnuplotPolarChartRenderer
    : public Base
{
  const ChartData* chartData_;

  void gnuplotCommand(gnuplotio::Gnuplot& gp) const override
  {
   gp<<this->chartData_->addinit_<<";";
   gp<<"unset border;"
       " set polar;"
       " set theta counterclockwise bottom;"
       " set grid polar 45.*pi/180.;"
       " set trange [0:2.*pi];"
       " set key rmargin;"
       " set size square;"
       " unset xtics;"
       " unset ytics;"
       ;

   double rmax=0.;
   for ( const PlotCurve& pc: this->chartData_->plc_ ) {
    rmax=std::max(rmax, pc.xy().col(1).max());
   }

   gp<<"set_label(x, text) = sprintf(\"set label '%s' at polar (%f), ("<<rmax<<") center\", text, x);"
    <<"eval set_label(0, \"$0^\\\\circ$\");"
    <<"eval set_label(45.*pi/180., \"$45^\\\\circ$\");"
    //<<"eval set_label(90.*pi/180., \"$90^\\\\circ$\");"
    <<"eval set_label(135.*pi/180., \"$135^\\\\circ$\");"
    <<"eval set_label(180.*pi/180., \"$180^\\\\circ$\");"
    <<"eval set_label(225.*pi/180., \"$225^\\\\circ$\");"
    <<"eval set_label(270.*pi/180., \"$270^\\\\circ$\");"
    <<"eval set_label(315.*pi/180., \"$315^\\\\circ$\");"
    <<"set rlabel '"<<this->chartData_->ylabel_<<"';"
      ;

   //gp<<"set xlabel '"<<xlabel_<<"'; set ylabel '"<<ylabel_<<"'; ";

   if ( this->chartData_->plc_.size() >0 )
   {
    gp<<"plot ";
    bool is_first=true;

    for ( const PlotCurve& pc: this->chartData_->plc_ )
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

    for ( const PlotCurve& pc: this->chartData_->plc_ )
    {
     if ( pc.xy_.n_rows>0 )
     {
      arma::mat xy = pc.xy_;
      xy.col(0) *= this->phi_unit_;
      gp.send1d ( xy );
     }
    }

   }
  }

public:
  GnuplotPolarChartRenderer(const ChartData* data, double phi_unit)
      : Base(phi_unit),
        chartData_(data)
  {}
};


} // namespace insight

#endif // INSIGHT_GNUPLOTPOLARCHARTRENDERER_H
