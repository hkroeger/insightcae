#include "qcomment.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>

namespace insight
{

defineType(QComment);
addToFactoryTable(IQResultElement, QComment);

QComment::QComment(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element )
    : IQResultElement(parent, hdmodel, element)
{}

QVariant QComment::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
  {
    auto &comment = elementAs<insight::Comment>();

    auto plaintextcomment = SimpleLatex(comment.value()).toPlainText();
    QString excerpt=QString::fromStdString(plaintextcomment);
    int cutoff=30;
    if (excerpt.size()>cutoff)
    {
      excerpt.replace(cutoff, excerpt.size()-cutoff, "...");
    }

    return excerpt;
  }

  return QVariant();
}

void QComment::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);
  te_=new IQSimpleLatexView(
      elementAs<insight::Comment>().value());
  layout->addWidget(te_);
}


}
