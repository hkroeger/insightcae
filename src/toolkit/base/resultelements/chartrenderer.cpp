#include "chartrenderer.h"

#include "base/boost_include.h"

#ifdef CHART_RENDERER_GNUPLOT
#include "base/resultelements/gnuplotrenderer.h"
#endif

#if defined(CHART_RENDERER_MATPLOTLIB)
#include "base/resultelements/matplotlibrenderer.h"
#endif


namespace insight
{


ChartRenderer::ChartRenderer(const ChartData *data)
  : chartData_(data)
{}



std::unique_ptr<ChartRenderer> ChartRenderer::create(const ChartData *data)
{
  std::unique_ptr<ChartRenderer> renderer;

  std::string selectedRenderer="gnuplot";

#if !defined(CHART_RENDERER_GNUPLOT) && defined(CHART_RENDERER_MATPLOTLIB)
  selectedRenderer="matplotlib";
#endif


  if (const auto* rv=getenv("INSIGHT_CHARTRENDERER"))
    selectedRenderer=rv;

#ifdef CHART_RENDERER_GNUPLOT
  if (selectedRenderer=="gnuplot")
    renderer.reset( new GnuplotRenderer(data) );
#endif

#if defined(CHART_RENDERER_MATPLOTLIB)
  if (selectedRenderer=="matplotlib")
    renderer.reset( new MatplotlibRenderer(data) );
#endif

  if (!renderer)
    throw insight::Exception("Could not instantiate chart renderer \""+selectedRenderer+"\"");

  return renderer;
}



}
