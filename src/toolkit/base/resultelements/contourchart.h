#ifndef INSIGHT_CONTOURCHART_H
#define INSIGHT_CONTOURCHART_H


#include "base/resultelements/chart.h"

namespace insight {



struct PlotField {
    arma::mat xy_;
    std::string plotcmd_;

    PlotField();
    PlotField ( const arma::mat& xy, const std::string& plotcmd = "" );
};




typedef std::vector<PlotCurve> PlotFieldList;




void addContourPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotFieldList& plc,
    const std::string& shortDescription,
    const std::string& addinit = ""
);



} // namespace insight

#endif // INSIGHT_CONTOURCHART_H
