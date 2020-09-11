#include "qcomment.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>

namespace insight
{

defineType(QComment);
addToFactoryTable(QResultElement, QComment);

QComment::QComment(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : QResultElement(parent, label, rep)
{

}

QVariant QComment::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
  {
    auto comment = resultElementAs<insight::Comment>();

    auto plaintextcomment = SimpleLatex(comment->value()).toPlainText();
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
  QResultElement::createFullDisplay(layout);
  te_=new QTextEdit;
  te_->setReadOnly(true);
  te_->setFrameShape(QFrame::NoFrame);
  layout->addWidget(te_);
}

void QComment::resetContents(int width, int height)
{
  QResultElement::resetContents(width, height);
  auto comment = resultElementAs<insight::Comment>();
  te_->setHtml(QString::fromStdString( SimpleLatex(comment->value()).toHTML(width) ));
}


}
