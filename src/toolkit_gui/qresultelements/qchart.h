#ifndef INSIGHT_QCHART_H
#define INSIGHT_QCHART_H

#include "toolkit_gui_export.h"


#include "qimage.h"

namespace insight {

class TOOLKIT_GUI_EXPORT QChart
 : public QImage
{
    Q_OBJECT

public:
    declareType ( insight::Chart::typeName_() );

    QChart(QObject* parent, const QString& label, insight::ResultElementPtr rep);
};

} // namespace insight

#endif // INSIGHT_QCHART_H
