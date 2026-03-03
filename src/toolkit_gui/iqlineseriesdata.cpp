#include "iqlineseriesdata.h"

#include <cfloat>

#include <QRandomGenerator>




auto* globalRanGen = QRandomGenerator::system();




ChartBounds::ChartBounds()
    : xmin(DBL_MAX), ymin(DBL_MAX),
    xmax(-DBL_MAX), ymax(-DBL_MAX)
{}




void ChartBounds::update(double x, double y)
{
    xmin=std::min(xmin, x);
    ymin=std::min(ymin, y);
    xmax=std::max(xmax, x);
    ymax=std::max(ymax, y);
}




void ChartBounds::update(const ChartBounds &o)
{
    xmin=std::min(xmin, o.xmin);
    ymin=std::min(ymin, o.ymin);
    xmax=std::max(xmax, o.xmax);
    ymax=std::max(ymax, o.ymax);
}




IQLineSeriesData::IQLineSeriesData(
    const QString& name,
    QtCharts::QChart* chart,
    IQChartInteractiveLegend *legend
    )
    : legend_(legend)
{
    crv=new QtCharts::QLineSeries(this);

    crv->setName(name);
    QColor c(
        globalRanGen->generateDouble()*255.0,
        globalRanGen->generateDouble()*255.0,
        globalRanGen->generateDouble()*255.0 );
    crv->setPen(QPen(c, 2.0));
    chart->addSeries(crv);
    auto vax=chart->axes(Qt::Vertical);
    crv->attachAxis(vax[0]);
    auto hax=chart->axes(Qt::Horizontal);
    crv->attachAxis(hax[0]);

    legend_->addEntry(name, this);

}




void IQLineSeriesData::append(double x, double y)
{
    values.append(QPointF(x,y));
    b_.update(x, y);
}




void IQLineSeriesData::updateLineSeries(int maxResolution)
{
    QList<QPointF> ptDisplay;

    int nv=values.size();
    int i=0;
    if (nv>maxRecent)
    {
        int nx = nv-1-maxRecent;
        int spc = std::max<int>(1, floor(double(nx)/double(maxResolution-maxRecent)));
        for (;i<nx;i+=spc)
            ptDisplay.append(values.at(i));
    }
    for(;i<nv;++i)
        ptDisplay.append(values.at(i));

    crv->replace(ptDisplay);
}
