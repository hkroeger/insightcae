#include "sdmdiarea.h"

#include <QPainter>

SDMdiArea::SDMdiArea(QWidget* parent)
  : QMdiArea(parent),
    m_pixmap(":workbench/resources/workbench_startUp.png")
{
  setBackground(QBrush(Qt::white));
}



void SDMdiArea::paintEvent(QPaintEvent *event)
{
  QMdiArea::paintEvent(event);

  QPainter painter(viewport());

  // Calculate the logo position - the bottom right corner of the mdi area.
//  int x = std::max(0, width() - m_pixmap.width() - 10);
//  int y = std::max(0, height() - m_pixmap.height() - 10);
  int x=0, y=0;
  painter.setOpacity(0.3);
  painter.drawPixmap(x, y, width(), m_pixmap.height()*width()/m_pixmap.width(), m_pixmap);
}

