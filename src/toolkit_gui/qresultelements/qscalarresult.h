#ifndef INSIGHT_QSCALARRESULT_H
#define INSIGHT_QSCALARRESULT_H

#include "qresultsetmodel.h"

namespace insight {

class QScalarResult : public QResultElement
{
    Q_OBJECT

public:
    declareType ( insight::ScalarResult::typeName_() );

    QScalarResult(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};

} // namespace insight

#endif // INSIGHT_QSCALARRESULT_H
