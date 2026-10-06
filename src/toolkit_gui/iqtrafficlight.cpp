#include "iqtrafficlight.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>


IQTrafficLight::IQTrafficLight(QWidget* parent, Qt::Orientation o)
    : QWidget(parent),
    state_(Unknown),
    orientation_(o)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}


IQTrafficLight::State IQTrafficLight::state() const
{
    return state_;
}


void IQTrafficLight::setState(State s)
{
    if (s!=state_)
    {
        state_=s;
        update();
    }
}


Qt::Orientation IQTrafficLight::orientation() const
{
    return orientation_;
}


void IQTrafficLight::setOrientation(Qt::Orientation o)
{
    if (o!=orientation_)
    {
        orientation_=o;
        updateGeometry();
        update();
    }
}


QSize IQTrafficLight::sizeHint() const
{
    return orientation_==Qt::Vertical ?
               QSize(18, 46) : QSize(46, 18);
}


QSize IQTrafficLight::minimumSizeHint() const
{
    return sizeHint();
}


void IQTrafficLight::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF housing = QRectF(rect()).adjusted(1, 1, -1, -1);
    bool vertical = (orientation_==Qt::Vertical);
    double thickness = vertical ? housing.width() : housing.height();
    double length = vertical ? housing.height() : housing.width();
    double radius = thickness/2.;

    p.setPen(QPen(QColor(30, 30, 30), 1));
    p.setBrush(QColor(55, 55, 55));
    p.drawRoundedRect(housing, radius, radius);

    struct Lamp { State state; QColor on; };
    const Lamp lamps[3] = {
        { Red,    QColor(230,  40,  40) },
        { Yellow, QColor(245, 200,  30) },
        { Green,  QColor( 40, 200,  60) }
    };

    double d = std::min(thickness-4., (length-8.)/3.); // lamp diameter
    double gap = (length-3.*d)/4.;
    for (int i=0; i<3; ++i)
    {
        double along = gap+i*(d+gap);
        double across = (thickness-d)/2.;
        QRectF lr = vertical ?
            QRectF(housing.left()+across, housing.top()+along, d, d) :
            QRectF(housing.left()+along, housing.top()+across, d, d);

        bool lit = (state_==lamps[i].state);
        QColor c = lit ? lamps[i].on : lamps[i].on.darker(state_==Unknown ? 400 : 300);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(lr);

        if (lit)
        {
            // small highlight
            p.setBrush(QColor(255, 255, 255, 90));
            p.drawEllipse(lr.adjusted(d*0.2, d*0.15, -d*0.45, -d*0.5));
        }
    }
}
