#include "iqgraphprogresschart.h"

#include <cmath>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>


#include <QTimer>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QValueAxis>




IQGraphProgressChart::IQGraphProgressChart(
    bool logscale,
    QWidget* parent
    )
  : QWidget(parent),

    legend_(new IQChartInteractiveLegend),
    chartView_(new QtCharts::QChartView),
    chartData_(new QtCharts::QChart),

    maxCnt_(8000),
    needsRedraw_(true),
    logscale_(logscale)
{
    auto *graphLayout = new QVBoxLayout;
    graphLayout->addWidget(chartView_, 1);
    graphLayout->addWidget(legend_);
    setLayout(graphLayout);

    chartView_->setChart(chartData_);

    chartView_->setBackgroundBrush( Qt::white );

    if (logscale_)
    {
        auto *ly = new QtCharts::QLogValueAxis();
        ly->setBase(10);
        chartData_->addAxis(ly, Qt::AlignLeft);
    }
    else
    {
        chartData_->addAxis(new QtCharts::QValueAxis, Qt::AlignLeft);
    }
    chartData_->addAxis(new QtCharts::QValueAxis, Qt::AlignBottom);
    chartData_->legend()->hide();

    auto hax=chartData_->axes(Qt::Horizontal);
    hax[0]->setGridLineVisible(true);
    auto vax=chartData_->axes(Qt::Vertical);
    vax[0]->setGridLineVisible(true);

    QTimer *timer=new QTimer;
    connect(timer, &QTimer::timeout, this, &IQGraphProgressChart::checkForUpdate);
    timer->setInterval(1000);
    timer->start();
}




IQGraphProgressChart::~IQGraphProgressChart()
{}




void IQGraphProgressChart::update(double iter, const std::string& name, double y_value)
{
    mutex_.lock();
    setUpdatesEnabled(false);


    std::shared_ptr<IQLineSeriesData> crv;
    auto i = curve_.find(name);
    if (i!=curve_.end())
    {
        crv=i->second;
    }
    else
    {
        crv=std::make_shared<IQLineSeriesData>(QString::fromStdString(name), chartData_, legend_);
        curve_[name]=crv;
    }

    if ( !logscale_ || ( logscale_ && (y_value > 0.0) ) ) // only add, if y>0. Plot gets unreadable otherwise
    {
        crv->append(iter, y_value);
    }

    //  setAxisAutoScale(QwtPlot::yLeft);
    needsRedraw_=true;

    setUpdatesEnabled(true);
    mutex_.unlock();
}




void IQGraphProgressChart::reset()
{
    for ( CurveList::value_type& i: curve_)
    {
        i.second.reset();
    }
    curve_.clear();
    needsRedraw_=true;
}




void IQGraphProgressChart::checkForUpdate()
{
    mutex_.lock();

    if (needsRedraw_)
    {
        needsRedraw_=false;

        ChartBounds b;
        for (auto& sd: curve_)
        {
            sd.second->updateLineSeries(width());

            if (sd.second->isVisible())
            {
                b.update(sd.second->bounds());
            }
        }

        if (fabs(b.xmax-b.xmin)<1e-20) { b.xmax=b.xmin+1e-4; } // all values the same
        if (b.xmin>b.xmax) { b.xmin=0; b.xmax=1.; } // no values

        if (fabs(b.ymax-b.ymin)<1e-20) { b.ymax=b.ymin+1e-4; }
        if (b.ymin>b.ymax) { b.ymin=0; b.ymax=1.; }

        auto hax=chartData_->axes(Qt::Horizontal);
        hax[0]->setRange(b.xmin, 1.05*b.xmax);

        double delta=fabs(b.ymax-b.ymin);
        auto vax=chartData_->axes(Qt::Vertical);
        vax[0]->setRange(b.ymin - (logscale_?0.0:0.05*delta), b.ymax+0.05*delta);

        repaint();
    }

    mutex_.unlock();
}


QColor IQLineSeriesData::color() const
{
    return crv->color();
}

bool IQLineSeriesData::isVisible() const
{
    return crv->isVisible();
}

void IQLineSeriesData::setVisibility(bool visible)
{
    crv->setVisible(visible);
}


