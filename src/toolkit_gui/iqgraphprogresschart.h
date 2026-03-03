#ifndef IQGRAPHPROGRESSCHART_H
#define IQGRAPHPROGRESSCHART_H

#include "toolkit_gui_export.h"
#include "iqlineseriesdata.h"

#include <QMutex>
#include <QLineEdit>

#include <QtCharts/QChartView>




class TOOLKIT_GUI_EXPORT IQGraphProgressChart
    : public QWidget
{
    Q_OBJECT

public:
    typedef std::map<std::string, std::shared_ptr<IQLineSeriesData> > CurveList;

protected:
    QtCharts::QChartView *chartView_;
    QtCharts::QChart *chartData_;
    IQChartInteractiveLegend *legend_;

    QLineEdit *fromx, *tox;

    size_t maxCnt_;

    CurveList curve_;
    bool needsRedraw_;
    QMutex mutex_;
    bool logscale_;

public:
    IQGraphProgressChart(bool logscale, QWidget* parent);
    ~IQGraphProgressChart();

    virtual void update(double t, const std::string& name, double y_value);

public Q_SLOTS:
    virtual void reset();
    void exportToCSV();

public slots:
    void checkForUpdate();
};


#endif // IQGRAPHPROGRESSCHART_H
