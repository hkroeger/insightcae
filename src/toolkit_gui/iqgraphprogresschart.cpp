#include "iqgraphprogresschart.h"
#include "base/exception.h"
#include "base/translations.h"

#include "base/tools.h"
#include "qtextensions.h"

#include <cmath>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>


#include <QTimer>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QValueAxis>
#include <fstream>




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
    auto *toplayout=new QHBoxLayout;

    auto clearbtn=new QPushButton(_("Clear"));
    connect(
        clearbtn, &QPushButton::clicked,
        this, &IQGraphProgressChart::reset );

    auto exportbtn=new QPushButton(_("Export..."));
    connect(
        exportbtn, &QPushButton::clicked,
        this, &IQGraphProgressChart::exportToCSV );

    toplayout->addWidget(clearbtn);
    toplayout->addWidget(exportbtn);
    fromx=new QLineEdit("");
    fromx->setPlaceholderText("(minimum)");
    fromx->setClearButtonEnabled(true);
    tox=new QLineEdit("");
    tox->setPlaceholderText("(maximum)");
    tox->setClearButtonEnabled(true);
    toplayout->addWidget(clearbtn);
    toplayout->addWidget(new QLabel(_("X from:")));
    toplayout->addWidget(fromx);
    toplayout->addWidget(new QLabel(_("to:")));
    toplayout->addWidget(tox);
    toplayout->addItem(
        new QSpacerItem(
            10,10,
            QSizePolicy::Expanding,
            QSizePolicy::Minimum ));

    graphLayout->addLayout(toplayout);
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




void IQGraphProgressChart::exportToCSV()
{
    if (auto fn = getFileName(
            nullptr, "Save curves to file",
            GetFileMode::Save,
            {{ "csv", "Comma separated values" }} ) )
    {
        auto file = insight::ensureFileExtension(fn, "csv");

        for ( auto& i: curve_)
        {
            boost::filesystem::path thisfn =
                file.parent_path() /
                (file.filename().stem().string()
                    +"_"+insight::sanitizeStringForFileName(i.first).string()
                    +file.filename().extension().string());

            std::ofstream f(thisfn.string());
            insight::assertion(
                f.good(),
                _("Failed to open file %s for writing"),
                thisfn.c_str() );

            i.second->exportToCSV(f);
        }
    }
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

        b.xmax*=1.05;
        double delta=fabs(b.ymax-b.ymin);
        b.ymin=b.ymin - (logscale_?0.0:0.05*delta);
        b.ymax=b.ymax+0.05*delta;

        {
            auto t=fromx->text();
            if (!t.isEmpty())
            {
                bool ok=false;
                double v=t.toDouble(&ok);
                if (ok) b.xmin=v;
            }
        }
        {
            auto t=tox->text();
            if (!t.isEmpty())
            {
                bool ok=false;
                double v=t.toDouble(&ok);
                if (ok) b.xmax=v;
            }
        }

        auto hax=chartData_->axes(Qt::Horizontal);
        hax[0]->setRange(b.xmin, b.xmax);

        auto vax=chartData_->axes(Qt::Vertical);
        vax[0]->setRange(b.ymin, b.ymax);

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




void IQLineSeriesData::exportToCSV(std::ostream &os) const
{
    for (int i=0; i<crv->count(); ++i)
    {
        os<<crv->at(i).x()<<" "<<crv->at(i).y()<<"\n";
    }
}




void IQLineSeriesData::setVisibility(bool visible)
{
    crv->setVisible(visible);
}


