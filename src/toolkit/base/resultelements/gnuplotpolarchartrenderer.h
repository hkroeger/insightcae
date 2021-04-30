#ifndef INSIGHT_GNUPLOTPOLARCHARTRENDERER_H
#define INSIGHT_GNUPLOTPOLARCHARTRENDERER_H

#include "polarchartrenderer.h"
#include "base/resultelements/gnuplotchartrenderer.h"

namespace insight {

class GnuplotPolarChartRenderer
    : public GnuplotRendererBase<PolarChartRenderer>
{
  void gnuplotCommand(gnuplotio::Gnuplot&) const override;

public:
  GnuplotPolarChartRenderer(const ChartData* data, double phi_unit);
};


} // namespace insight

#endif // INSIGHT_GNUPLOTPOLARCHARTRENDERER_H
