#ifndef QCOMMENT_H
#define QCOMMENT_H

#include "qresultsetmodel.h"

class QTextEdit;

namespace insight
{


class QComment
: public QResultElement
{
    Q_OBJECT

    QTextEdit *te_;

public:
    declareType ( insight::Comment::typeName_() );

    QComment(QObject* parent, const QString& label, insight::ResultElementPtr rep);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
    void resetContents(int width, int height) override;
};


}

#endif // QCOMMENT_H
