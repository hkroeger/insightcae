#ifndef INSIGHT_QSCALARRESULT_H
#define INSIGHT_QSCALARRESULT_H

#include "toolkit_gui_export.h"


#include "iqresultsetmodel.h"

namespace insight {

class TOOLKIT_GUI_EXPORT QScalarResult : public IQResultElement
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
