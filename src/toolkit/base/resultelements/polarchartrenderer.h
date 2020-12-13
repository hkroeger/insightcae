#ifndef INSIGHT_POLARCHARTRENDERER_H
#define INSIGHT_POLARCHARTRENDERER_H

#include "base/resultelements/chartrenderer.h"

namespace insight {



class PolarChartRenderer
    : public ChartRenderer
{
protected:
  double phi_unit_;

public:
  PolarChartRenderer(const ChartData* data, double phi_unit);
};



} // namespace insight

#endif // INSIGHT_POLARCHARTRENDERER_H
