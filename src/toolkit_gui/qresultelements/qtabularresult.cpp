#include "qtabularresult.h"
#include "qtextensions.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidget>

namespace insight {

defineType(QTabularResult);
addToFactoryTable(IQResultElement, QTabularResult);

QTabularResult::QTabularResult(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element )
    : IQResultElement(parent, hdmodel, element)
{
}


QVariant QTabularResult::previewInformation(int) const
{
    return QVariant();
}


void QTabularResult::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);

  auto &res=elementAs<TabularResult>();

  auto tw=new QTableWidget(res.rows().size(), res.headings().size()/*, this*/);

  QStringList headers;
  for (auto& h: res.headings() )
  {
    headers << QString::fromStdString(h.toPlainText());
  }
  tw->setHorizontalHeaderLabels( headers );
  tw->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  tw->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  for (size_t i=0; i<res.rows().size(); i++)
  {
    const std::vector<double>& row=res.rows()[i];
    for (size_t j=0; j<row.size(); j++)
    {
      tw->setItem(i, j, new QTableWidgetItem( QString::number(row[j]) ));
    }
  }
  tw->doItemsLayout();
  tw->resizeColumnsToContents();

  layout->addWidget(tw);
  tw->setMaximumHeight(availableContentHeight(tw));
}

} // namespace insight
