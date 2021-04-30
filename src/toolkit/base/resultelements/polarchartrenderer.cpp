#include "polarchartrenderer.h"



namespace insight {



PolarChartRenderer::PolarChartRenderer(const ChartData *data, double phi_unit)
  : ChartRenderer(data),
    phi_unit_(phi_unit)
{}




} // namespace insight
