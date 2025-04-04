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


class QAttributeTableView
    : public QTableWidget
{
    insight::AttributeTableResult* tab_;

    void updateContent()
    {
        auto width=viewport()->width();
      QStringList headers;
      headers
              << QString::fromStdString(tab_->labelColumnTitle().toHTML(width))
              << QString::fromStdString(tab_->valueColumnTitle().toHTML(width));
      setHorizontalHeaderLabels( headers );

      for (size_t i=0; i<tab_->names().size(); i++)
      {
          auto attr_name = QString::fromStdString(tab_->names()[i].toHTML(width));
          QString attr_val(boost::lexical_cast<std::string>(tab_->values()[i]).c_str());
          setItem(i, 0, new QTableWidgetItem( attr_name ));
          setItem(i, 1, new QTableWidgetItem( attr_val ));
      }
      doItemsLayout();
      resizeColumnsToContents();
    };

public:
    QAttributeTableView(insight::AttributeTableResult* tab)
     : QTableWidget(tab->names().size(), 2),
       tab_(tab)
    {
        // setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void resizeEvent(QResizeEvent *e)
    {
        QTableWidget::resizeEvent(e);
        updateContent();
    }
};


void QAttributeResultTable::createFullDisplay(QVBoxLayout* layout)
{
  auto res = resultElementAs<insight::AttributeTableResult>();
  IQResultElement::createFullDisplay(layout);

  tw_ = new QAttributeTableView(res);
  layout->addWidget(tw_);
}




} // namespace insight
