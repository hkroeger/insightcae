#ifndef INSIGHT_POLARCHART_H
#define INSIGHT_POLARCHART_H


#include "base/resultelements/chart.h"
#include "base/resultelements/chartrenderer.h"
#include "base/resultelements/latexgnuplotrenderer.h"
#include "base/units.h"

namespace insight {

class PolarChart
  : public Chart
{
 double phi_unit_;
public:
 declareType ( "PolarChart" );

 PolarChart ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
 PolarChart
 (
     const std::string& rlabel,
     const PlotCurveList& plc,
     const std::string& shortDesc,
     const std::string& longDesc,
     double phi_unit = SI::rad,
     const std::string& addinit = ""
 );

 void generatePlotImage ( const boost::filesystem::path& imagepath ) const override;

 ResultElementPtr clone() const override;
};


insight::ResultElement& addPolarPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& rlabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    double phi_unit = SI::rad,
    const std::string& addinit = "",
    const std::string& watermarktext = ""
);



} // namespace insight

#endif // INSIGHT_POLARCHART_H
