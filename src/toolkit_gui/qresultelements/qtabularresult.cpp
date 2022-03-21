#include "qtabularresult.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidget>

namespace insight {

defineType(QTabularResult);
addToFactoryTable(IQResultElement, QTabularResult);

QTabularResult::QTabularResult(QObject *parent, const QString &label, insight::ResultElementPtr rep)
: IQResultElement(parent, label, rep)
{
}


QVariant QTabularResult::previewInformation(int) const
{
    return QVariant();
}


void QTabularResult::createFullDisplay(QVBoxLayout* layout)
{
  IQResultElement::createFullDisplay(layout);

  auto resp=resultElementAs<TabularResult>();
  auto& res=*resp;

  auto tw=new QTableWidget(res.rows().size(), res.headings().size()/*, this*/);

  QStringList headers;
  for (const std::string& h: res.headings() )
  {
    headers << QString::fromStdString(h);
  }
  tw->setHorizontalHeaderLabels( headers );
  tw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tw->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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
}

} // namespace insight
