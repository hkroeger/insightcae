#include "qattributeresulttable.h"

#include <QVBoxLayout>
#include <QTableWidget>

namespace insight {



defineType(QAttributeResultTable);
addToFactoryTable(QResultElement, QAttributeResultTable);


QAttributeResultTable::QAttributeResultTable(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : QResultElement(parent, label, rep)
{

}

QVariant QAttributeResultTable::previewInformation(int) const
{
  return QVariant();
}

void QAttributeResultTable::createFullDisplay(QVBoxLayout* layout)
{
  auto res = resultElementAs<insight::AttributeTableResult>();
  QResultElement::createFullDisplay(layout);

  auto le_=new QTableWidget(res->names().size(), 2/*, this*/);
  le_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  le_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QStringList headers;
  headers << "Attribute" << "Value";
  le_->setHorizontalHeaderLabels( headers );

  for (size_t i=0; i<res->names().size(); i++)
  {
      QString attr_name(res->names()[i].c_str());
      QString attr_val(boost::lexical_cast<std::string>(res->values()[i]).c_str());
      le_->setItem(i, 0, new QTableWidgetItem( attr_name ));
      le_->setItem(i, 1, new QTableWidgetItem( attr_val ));
  }
  le_->doItemsLayout();
  le_->resizeColumnsToContents();

  layout->addWidget(le_);
}

} // namespace insight
