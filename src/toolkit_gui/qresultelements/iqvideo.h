#ifndef IQVIDEO_H
#define IQVIDEO_H

#include "toolkit_gui_export.h"


#include "iqresultsetmodel.h"

#include <QPixmap>

class QLabel;
class QScrollArea;

namespace insight {

class TOOLKIT_GUI_EXPORT IQVideo
    : public IQResultElement
{
    Q_OBJECT

    int delta_w_;
    QLabel *id_;
    QPixmap previewImage_;

protected:
    void setPreviewImage(const QPixmap& pm);

public:
    declareType ( insight::Video::typeName_() );

    IQVideo(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout *layout) override;
};

} // namespace insight

#endif // IQVIDEO_H
