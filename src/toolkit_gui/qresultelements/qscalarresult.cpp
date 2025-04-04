#include "qscalarresult.h"

#include <QVBoxLayout>
#include <QLabel>

namespace insight {

defineType(QScalarResult);
addToFactoryTable(IQResultElement, QScalarResult);

QScalarResult::QScalarResult(QObject *parent, const QString &label, insight::ResultElementPtr rep)
: IQResultElement(parent, label, rep)
{}


QVariant QScalarResult::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
  {
    auto res = resultElementAs<insight::ScalarResult>();
    return QString::number(res->value());
  }

  return QVariant();
}


void QScalarResult::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);

  auto res = resultElementAs<insight::ScalarResult>();
  auto te_=new QLabel;
  te_->setText(
      QString("<big><b>%1=%2 %3</b></big>")
          .arg(label_).arg(res->value())
          .arg(QString::fromStdString(
              res->unit().toHTML(50))) );
  layout->addWidget(te_);
}

} // namespace insight
