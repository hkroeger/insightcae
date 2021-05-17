#ifndef INSIGHT_QTABULARRESULT_H
#define INSIGHT_QTABULARRESULT_H

#include "toolkit_gui_export.h"


#include "qresultsetmodel.h"


namespace insight {

class TOOLKIT_GUI_EXPORT QTabularResult
        : public QResultElement
{
    Q_OBJECT

public:
    declareType ( insight::TabularResult::typeName_() );

    QTabularResult(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};

} // namespace insight

#endif // INSIGHT_QTABULARRESULT_H
