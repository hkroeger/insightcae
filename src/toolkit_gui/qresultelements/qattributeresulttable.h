#ifndef INSIGHT_QATTRIBUTERESULTTABLE_H
#define INSIGHT_QATTRIBUTERESULTTABLE_H

#include "toolkit_gui_export.h"


#include "iqresultsetmodel.h"

namespace insight {

class TOOLKIT_GUI_EXPORT QAttributeResultTable
  : public IQResultElement
{
    Q_OBJECT

public:
    declareType ( insight::AttributeTableResult::typeName_() );
    QAttributeResultTable(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};

} // namespace insight

#endif // INSIGHT_QATTRIBUTERESULTTABLE_H
