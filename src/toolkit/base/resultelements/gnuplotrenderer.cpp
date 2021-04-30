#include "gnuplotrenderer.h"

#include "base/resultelements/chart.h"
#include "base/resultelements/chartrenderer.h"

#include "gnuplot-iostream.h"



namespace insight
{


void GnuplotRenderer::gnuplotCommand(gnuplotio::Gnuplot& gp) const
{
 gp<<chartData_->addinit_<<";";
 gp<<"set xlabel '"<<chartData_->xlabel_<<"'; set ylabel '"<<chartData_->ylabel_<<"'; set grid; ";
 if ( chartData_->plc_.size() >0 )
 {
  gp<<"plot ";
  bool is_first=true;

  if (chartData_->plc_.include_zero)
  {
   gp<<"0 not lt -1";
   is_first=false;
  }

  for ( const PlotCurve& pc: chartData_->plc_ )
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

  for ( const PlotCurve& pc: chartData_->plc_ )
  {
   if ( pc.xy_.n_rows>0 )
   {
    gp.send1d ( pc.xy_ );
   }
  }

 }
}

GnuplotRenderer::GnuplotRenderer(const ChartData* data)
  : GnuplotRendererBase<ChartRenderer>(data)
{}


}
