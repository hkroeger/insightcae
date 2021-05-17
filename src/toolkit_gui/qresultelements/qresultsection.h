#ifndef INSIGHT_QRESULTSECTION_H
#define INSIGHT_QRESULTSECTION_H

#include "toolkit_gui_export.h"


#include "qresultsetmodel.h"

class QTextEdit;

namespace insight {

class TOOLKIT_GUI_EXPORT QResultSection
: public QResultElement
{
    Q_OBJECT

  QTextEdit* te_;

public:
    declareType ( insight::ResultSection::typeName_() );

    QResultSection(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
    void resetContents(int width, int height) override;
};

} // namespace insight

#endif // INSIGHT_QRESULTSECTION_H
