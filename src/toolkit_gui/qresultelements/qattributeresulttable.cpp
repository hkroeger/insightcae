#include "qattributeresulttable.h"

#include <QVBoxLayout>
#include <QTableWidget>

namespace insight {



defineType(QAttributeResultTable);
addToFactoryTable(IQResultElement, QAttributeResultTable);




QAttributeResultTable::QAttributeResultTable(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : IQResultElement(parent, label, rep)
{

}




QVariant QAttributeResultTable::previewInformation(int) const
{
  return QVariant();
}




void QAttributeResultTable::createFullDisplay(QVBoxLayout* layout)
{
  auto res = resultElementAs<insight::AttributeTableResult>();
  IQResultElement::createFullDisplay(layout);

  tw_ = new QTableWidget(res->names().size(), 2/*, this*/);
  tw_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tw_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  layout->addWidget(tw_);
}




void QAttributeResultTable::resetContents(int width, int height)
{
  auto res = resultElementAs<insight::AttributeTableResult>();

  QStringList headers;
  headers
          << QString::fromStdString(res->labelColumnTitle().toHTML(width))
          << QString::fromStdString(res->valueColumnTitle().toHTML(width));
  tw_->setHorizontalHeaderLabels( headers );

  for (size_t i=0; i<res->names().size(); i++)
  {
      auto attr_name = QString::fromStdString(res->names()[i].toHTML(width));
      QString attr_val(boost::lexical_cast<std::string>(res->values()[i]).c_str());
      tw_->setItem(i, 0, new QTableWidgetItem( attr_name ));
      tw_->setItem(i, 1, new QTableWidgetItem( attr_val ));
  }
  tw_->doItemsLayout();
  tw_->resizeColumnsToContents();
}



} // namespace insight
