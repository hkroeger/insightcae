#include "qchart.h"

namespace insight {


defineType(QChart);
addToFactoryTable(QResultElement, QChart);


QChart::QChart(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : QImage(parent, label, rep)
{
    if (auto im = resultElementAs<insight::Chart>())
    {
        auto chart_file_=boost::filesystem::unique_path(boost::filesystem::temp_directory_path()/"%%%%-%%%%-%%%%-%%%%.png");
        im->generatePlotImage(chart_file_);
        setImage( QPixmap(QString::fromStdString(chart_file_.string())) );
    }
}

} // namespace insight
