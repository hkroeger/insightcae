#ifndef INSIGHT_QRESULTSECTION_H
#define INSIGHT_QRESULTSECTION_H

#include "qresultsetmodel.h"

class QTextEdit;

namespace insight {

class QResultSection
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
