#ifndef SDMDIAREA_H
#define SDMDIAREA_H

#include "toolkit_gui_export.h"


#include <QMdiArea>

class TOOLKIT_GUI_EXPORT SDMdiArea
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
