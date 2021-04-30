#ifndef GNUPLOTRENDERER_H
#define GNUPLOTRENDERER_H


#include "base/resultelements/chartrenderer.h"
#include "base/resultelements/gnuplotchartrenderer.h"


namespace insight
{


class GnuplotRenderer
    : public GnuplotRendererBase<ChartRenderer>
{

  void gnuplotCommand(gnuplotio::Gnuplot&) const override;

public:
  GnuplotRenderer(const ChartData* data);

};



}


#endif // GNUPLOTRENDERER_H
