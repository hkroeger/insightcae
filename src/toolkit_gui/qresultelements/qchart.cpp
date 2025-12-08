#include "qchart.h"

namespace insight {


defineType(QChart);
addToFactoryTable(IQResultElement, QChart);


QChart::QChart(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element )
  : QImage(parent, hdmodel, element)
{
    auto &im = elementAs<insight::Chart>();
    {
        auto chart_file_ =
            boost::filesystem::unique_path(boost::filesystem::temp_directory_path()/"%%%%-%%%%-%%%%-%%%%.png");
        im.generatePlotImage(chart_file_);
        setImage( std::make_unique<QPixmap>(QString::fromStdString(chart_file_.string())) );
    }
}

} // namespace insight
