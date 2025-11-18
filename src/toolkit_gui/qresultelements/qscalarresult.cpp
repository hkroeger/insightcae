#include "qscalarresult.h"

#include <QVBoxLayout>
#include <QLabel>

namespace insight {

defineType(QScalarResult);
addToFactoryTable(IQResultElement, QScalarResult);

QScalarResult::QScalarResult(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element )
    : IQResultElement(parent, hdmodel, element)
{}


QVariant QScalarResult::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
  {
    auto &res = elementAs<insight::ScalarResult>();
    return QString::number(res.value());
  }

  return QVariant();
}


void QScalarResult::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);

  auto &res = elementAs<insight::ScalarResult>();
  auto te_=new QLabel;
  te_->setText(
      QString("<big><b>%1=%2 %3</b></big>")
          .arg(name()).arg(res.value())
          .arg(QString::fromStdString(
              res.unit().toHTML(50))) );
  layout->addWidget(te_);
}

} // namespace insight
