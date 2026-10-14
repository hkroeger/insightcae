#include "iqremoteserverstatus.h"

#include "base/exception.h"
#include "base/translations.h"

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QPointer>

#include <algorithm>
#include <cmath>
#include <thread>


namespace IQRemoteServerStatus
{


QIcon icon(State state, double utilization)
{
    QPixmap pm(12, 12);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(1.5, 1.5, 9., 9.);
    switch (state)
    {
    case State::Online:
        p.setPen(QPen(QColor("#1d6b2d"), 1.));
        p.setBrush(QColor("#2e9e44"));
        break;
    case State::Offline:
        p.setPen(QPen(QColor("#404040"), 1.5));
        p.setBrush(Qt::NoBrush);
        break;
    case State::Unknown:
        p.setPen(QPen(QColor("#a0a0a0"), 1., Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        break;
    case State::Busy:
        {
            // pie slice from 12 o'clock, clockwise (angles in 1/16 degree)
            double u = std::max(0., std::min(1., utilization));

            // colour by utilization: yellow, orange (>50%), red (>75%)
            QColor fill("#e8b100"), outline("#8a6900");
            if (u>0.75)
            {
                fill=QColor("#d93025"); outline=QColor("#8c1d16");
            }
            else if (u>0.5)
            {
                fill=QColor("#f07c00"); outline=QColor("#9a4f00");
            }

            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            if (u>=0.999)
                p.drawEllipse(r);
            else if (u>0.)
                p.drawPie(r, 90*16, -int(std::round(u*360.*16.)));
            // outline
            p.setPen(QPen(outline, 1.));
            p.setBrush(Qt::NoBrush);
        }
        break;
    }
    p.drawEllipse(r);

    return QIcon(pm);
}




bool isBusy(const insight::RemoteServer::Config::ProcessorLoad& load)
{
    return load.nFullyOccupied>0 || load.utilization>0.5;
}




QString describe(const insight::RemoteServer::Config::ProcessorLoad& load)
{
    auto loadInfo =
        QString(_("%1 of %2 cores fully occupied, %3% utilization"))
            .arg(load.nFullyOccupied)
            .arg(load.nProcessors)
            .arg(int(std::round(load.utilization*100.)));

    if (isBusy(load))
        return QString(_("online, busy: %1")).arg(loadInfo);
    else
        return QString(_("online: %1")).arg(loadInfo);
}




void checkInBackground(
    insight::RemoteServer::ConfigPtr cfg,
    QObject* receiver,
    std::function<void(bool online)> onReachability,
    std::function<void(const insight::RemoteServer::Config::ProcessorLoad&)> onLoad,
    std::function<void(bool exists)> onExistence )
{
    // the guard is only dereferenced in the GUI thread
    QPointer<QObject> guard(receiver);
    std::thread(
        [cfg, guard, onReachability, onLoad, onExistence]()
        {
            bool online=false;
            try
            {
                online = cfg->isRunning(); // ssh connect may take some seconds
            }
            catch (const std::exception& e)
            {
                insight::dbg()<<"checking server "<<static_cast<const std::string&>(*cfg)
                               <<" failed: "<<e.what()<<std::endl;
            }
            QMetaObject::invokeMethod(
                qApp,
                [guard, onReachability, online]()
                {
                    if (guard && onReachability) onReachability(online);
                },
                Qt::QueuedConnection );

            // not reachable: does the instance exist at all?
            if (!online && onExistence && cfg->canBeCreated())
            {
                if (auto ex = cfg->exists())
                {
                    bool e = *ex;
                    QMetaObject::invokeMethod(
                        qApp,
                        [guard, onExistence, e]()
                        {
                            if (guard) onExistence(e);
                        },
                        Qt::QueuedConnection );
                }
            }

            // then evaluate the processor load (takes some time)
            if (online && onLoad)
            {
                if (auto load = cfg->processorLoad())
                {
                    auto l = *load;
                    QMetaObject::invokeMethod(
                        qApp,
                        [guard, onLoad, l]()
                        {
                            if (guard) onLoad(l);
                        },
                        Qt::QueuedConnection );
                }
            }
        }
    ).detach();
}


}
