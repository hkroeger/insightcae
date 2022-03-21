#ifndef INSIGHT_QIMAGE_H
#define INSIGHT_QIMAGE_H

#include "toolkit_gui_export.h"


#include "iqresultsetmodel.h"

#include <QPixmap>

class QLabel;
class QScrollArea;

namespace insight {

class TOOLKIT_GUI_EXPORT QImage
: public IQResultElement
{
  Q_OBJECT

  int delta_w_;
  QScrollArea* sa_;
  QLabel *id_;
  QPixmap image_;

protected:
  void setImage(const QPixmap& pm);

public:
  declareType ( insight::Image::typeName_() );

  QImage(QObject* parent, const QString& label, insight::ResultElementPtr rep);

  QVariant previewInformation(int role) const override;
  void createFullDisplay(QVBoxLayout *layout) override;
  void resetContents(int width, int height) override;
};

} // namespace insight

#endif // INSIGHT_QIMAGE_H
