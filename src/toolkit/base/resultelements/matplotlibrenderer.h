#ifndef INSIGHT_MATPLOTLIBRENDERER_H
#define INSIGHT_MATPLOTLIBRENDERER_H

#include "base/resultelements/chartrenderer.h"


namespace insight {

class MatplotlibRenderer
    : public ChartRenderer
{

public:
  MatplotlibRenderer(const ChartData* data);

  void render(const boost::filesystem::path& outimagepath) const override;
};

} // namespace insight

#endif // INSIGHT_MATPLOTLIBRENDERER_H
