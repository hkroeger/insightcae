#include "qvectorresult.h"

#include <QLabel>
#include <QVBoxLayout>

namespace insight {

defineType(QVectorResult);
addToFactoryTable(IQResultElement, QVectorResult);

QVectorResult::QVectorResult(QObject *parent, const QString &label, insight::ResultElementPtr rep)
: IQResultElement(parent, label, rep)
{
    auto res = resultElementAs<insight::VectorResult>();
    auto v=res->value();
    v_=QString("(%1 %2 %3)").arg(v(0)).arg(v(1)).arg(v(2));
}


QVariant QVectorResult::previewInformation(int role) const
{
  if (role==Qt::DisplayRole)
    return v_;

  return QVariant();
}


void QVectorResult::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);
  auto te_=new QLabel;
  te_->setText(v_);
  layout->addWidget(te_);
}

} // namespace insight
