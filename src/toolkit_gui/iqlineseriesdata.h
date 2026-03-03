#ifndef IQLINESERIESDATA_H
#define IQLINESERIESDATA_H

#include "base/boost_include.h"
#include "iqchartinteractivelegend.h"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>

struct ChartBounds
{
    double xmin, ymin, xmax, ymax;

    ChartBounds();

    void update(double x, double y);
    void update(const ChartBounds& o);
};




class IQLineSeriesData
    : public QObject
{
    Q_OBJECT

    const int maxRecent=100;

    QtCharts::QLineSeries* crv;

    QList<QPointF> values;

    IQChartInteractiveLegend *legend_;

    ChartBounds b_;

public:
    IQLineSeriesData(
        const QString& name,
        QtCharts::QChart* chartData,
        IQChartInteractiveLegend *legend );

    void append(double x, double y);
    void updateLineSeries(int maxResolution=1000);

    QColor color() const;
    bool isVisible() const;

    inline const ChartBounds& bounds() const { return b_; }

    void exportToCSV(std::ostream& os) const;

public Q_SLOTS:
    void setVisibility(bool visible);
};



#endif // IQLINESERIESDATA_H
