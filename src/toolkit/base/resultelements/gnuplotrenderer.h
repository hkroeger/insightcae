#ifndef GNUPLOTRENDERER_H
#define GNUPLOTRENDERER_H


#include "base/resultelements/chartrenderer.h"
#include "base/resultelements/latexgnuplotrenderer.h"


#include "base/resultelements/chart.h"
#include "base/resultelements/chartrenderer.h"

#include "gnuplot-iostream.h"


namespace insight
{

template<class Base>
class GnuplotRenderer
    : public Base
{

  void gnuplotCommand(gnuplotio::Gnuplot& gp) const override
  {
   gp<<this->chartData_->addinit_<<";";
   gp<<"set xlabel '"<<this->chartData_->xlabel_
    <<"'; set ylabel '"<<this->chartData_->ylabel_<<"'; set grid; ";
   if ( this->chartData_->plc_.size() >0 )
   {
    gp<<"plot ";
    bool is_first=true;

    if (this->chartData_->plc_.include_zero)
    {
     gp<<"0 not lt -1";
     is_first=false;
    }

    for ( const PlotCurve& pc: this->chartData_->plc_ )
    {
     if ( !pc.plotcmd_.empty() )
     {
      if (!is_first) { gp << ","; is_first=false; }
      if ( pc.xy_.n_rows>0 )
      {
       gp<<"'-' "<<pc.plotcmd_;
      } else
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
      gp.send1d ( pc.xy_ );
     }
    }

   }
  }

public:
  GnuplotRenderer(const ChartData* data)
  : Base(data)
  {}
};



}


#endif // GNUPLOTRENDERER_H
