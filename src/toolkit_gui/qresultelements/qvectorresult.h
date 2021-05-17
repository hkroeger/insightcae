#ifndef INSIGHT_QVECTORRESULT_H
#define INSIGHT_QVECTORRESULT_H

#include "toolkit_gui_export.h"


#include "qresultsetmodel.h"

namespace insight {

class TOOLKIT_GUI_EXPORT QVectorResult : public QResultElement
{
    Q_OBJECT

    QString v_;

public:
    declareType ( insight::VectorResult::typeName_() );

    QVectorResult(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};

} // namespace insight

#endif // INSIGHT_QVECTORRESULT_H
