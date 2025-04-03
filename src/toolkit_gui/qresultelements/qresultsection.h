#ifndef INSIGHT_QRESULTSECTION_H
#define INSIGHT_QRESULTSECTION_H

#include "toolkit_gui_export.h"


#include "iqresultsetmodel.h"
#include "qtextensions.h"

class QTextEdit;

namespace insight {

class TOOLKIT_GUI_EXPORT QResultSection
: public IQResultElement
{
    Q_OBJECT

  IQSimpleLatexView* te_;

public:
    declareType ( insight::ResultSection::typeName_() );

    QResultSection(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};

} // namespace insight

#endif // INSIGHT_QRESULTSECTION_H
