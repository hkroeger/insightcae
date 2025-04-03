#include "qresultsection.h"


#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>



namespace insight {



defineType(QResultSection);
addToFactoryTable(IQResultElement, QResultSection);



QResultSection::QResultSection(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : IQResultElement(parent, label, rep)
{}



QVariant QResultSection::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
  {
    auto sec=resultElementAs<insight::ResultSection>();

    auto pt=SimpleLatex(sec->secionName()).toPlainText();

    return QString::fromStdString(pt);
  }

  return QVariant();
}



void QResultSection::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);
  te_=new IQSimpleLatexView(
      resultElementAs<insight::ResultSection>()
          ->introduction());
  layout->addWidget(te_);
}



} // namespace insight
