#include "statustabwidget.h"

#include <QStylePainter>
#include <QStyleOptionTab>


StatusTabBar::StatusTabBar(QWidget* parent)
  : QTabBar(parent)
{}

void StatusTabBar::setTabStatusColor(int index, const QColor& c)
{
  if (index<0) return;
  if (index>=int(colors_.size())) colors_.resize(index+1);
  colors_[index]=c;
  update();
}

void StatusTabBar::paintEvent(QPaintEvent*)
{
  QStylePainter p(this);
  for (int i=0; i<count(); ++i)
  {
    QStyleOptionTab opt;
    initStyleOption(&opt, i);
    p.drawControl(QStyle::CE_TabBarTabShape, opt);

    if (i<int(colors_.size()) && colors_[i].isValid())
    {
      QColor c=colors_[i];
      bool selected = opt.state & QStyle::State_Selected;
      QRect r = opt.rect.adjusted(2, 2, -2, selected ? 0 : -1);
      if (!selected) c.setAlpha(150);
      p.fillRect(r, c);
      if (selected)
      {
        p.setPen(QPen(colors_[i].darker(160), 2));
        p.drawRect(r.adjusted(1, 1, -1, -1));
      }
    }

    p.drawControl(QStyle::CE_TabBarTabLabel, opt);
  }
}


StatusTabWidget::StatusTabWidget(QWidget* parent)
  : QTabWidget(parent),
    bar_(new StatusTabBar(this))
{
  setTabBar(bar_);
}

void StatusTabWidget::setTabStatusColor(int index, const QColor& c)
{
  bar_->setTabStatusColor(index, c);
}
