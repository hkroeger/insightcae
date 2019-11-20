#ifndef SDMDIAREA_H
#define SDMDIAREA_H

#include <QMdiArea>

class SDMdiArea
        : public QMdiArea
{
public:
  SDMdiArea(QWidget* parent=nullptr);

protected:
  void paintEvent(QPaintEvent *event);

private:
  // Store the logo image.
  QPixmap m_pixmap;
};

#endif // SDMDIAREA_H
