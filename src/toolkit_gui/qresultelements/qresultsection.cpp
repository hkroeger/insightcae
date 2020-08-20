#include "qresultsection.h"


#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>

namespace insight {

defineType(QResultSection);
addToFactoryTable(QResultElement, QResultSection);

QResultSection::QResultSection(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : QResultElement(parent, label, rep)
{

}

QVariant QResultSection::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
  {
    auto sec=resultElementAs<insight::ResultSection>();

    return QString::fromStdString(SimpleLatex(sec->secionName()).toPlainText());
  }

  return QVariant();
}

void QResultSection::createFullDisplay(QVBoxLayout* layout)
{
  QResultElement::createFullDisplay(layout);
  te_=new QTextEdit;
  te_->setReadOnly(true);
  te_->setFrameShape(QFrame::NoFrame);
  layout->addWidget(te_);
}

void QResultSection::resetContents(int width, int height)
{
  QResultElement::resetContents(width, height);
  auto sec = resultElementAs<insight::ResultSection>();
  te_->setHtml(QString::fromStdString( SimpleLatex(sec->introduction()).toHTML(width) ));
}

} // namespace insight
