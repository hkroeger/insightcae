#ifndef INSIGHT_QCHART_H
#define INSIGHT_QCHART_H

#include "qimage.h"

namespace insight {

class QChart
 : public QImage
{
    Q_OBJECT

public:
    declareType ( insight::Chart::typeName_() );

    QChart(QObject* parent, const QString& label, insight::ResultElementPtr rep);
};

} // namespace insight

#endif // INSIGHT_QCHART_H
