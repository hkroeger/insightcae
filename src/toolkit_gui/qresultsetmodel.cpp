#include "qresultsetmodel.h"


#include <QLabel>
#include <QVBoxLayout>
#include <QEvent>
#include <QResizeEvent>
#include <QTextEdit>
#include <QTreeView>
#include <QSpacerItem>
#include <QDebug>

namespace insight
{




ResizeEventNotifier::ResizeEventNotifier(QObject* parent)
  : QObject(parent)
{
}




bool ResizeEventNotifier::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Resize && obj==parent())
  {
      QResizeEvent *ev = static_cast<QResizeEvent*>(event);
      auto s=ev->size();
      emit resized(s.width(), s.height());
  }
  return QObject::eventFilter(obj, event);
}






defineType(QResultElement);
defineFactoryTable(
    QResultElement,
    LIST(QObject* parent, const QString& label, insight::ResultElementPtr rep),
    LIST(parent, label, rep)
);




QResultElement::QResultElement(QObject *parent, const QString& label, insight::ResultElementPtr rep)
    : QObject(parent),
      resultElement_(rep),
      label_(label)
{}




QResultElement *QResultElement::parentResultElement() const
{
  return dynamic_cast<QResultElement*>(parent());
}




ResultElement *QResultElement::resultElement() const
{
  return resultElement_.get();
}




void QResultElement::createFullDisplay(QVBoxLayout* layout)
{
  layout->addWidget(new QLabel("<b>"+label_+"</b>"));

  if (resultElement_)
  {
    if (!resultElement_->shortDescription().empty())
    {
      shortDesc_=new QTextEdit;
      shortDesc_->setReadOnly(true);
      shortDesc_->setFrameShape(QFrame::NoFrame);
      shortDesc_->setMinimumHeight (2*QFontMetrics(shortDesc_->font()).lineSpacing());
      layout->addWidget(shortDesc_);
    }
    if (!resultElement_->longDescription().empty())
    {
      longDesc_=new QTextEdit;
      longDesc_->setReadOnly(true);
      longDesc_->setFrameShape(QFrame::NoFrame);
      longDesc_->setMinimumHeight (2*QFontMetrics(longDesc_->font()).lineSpacing()) ;
      layout->addWidget(longDesc_);
    }
  }


  auto ef=new ResizeEventNotifier(layout->parentWidget());
  layout->parentWidget()->installEventFilter(ef);
  connect(ef, &ResizeEventNotifier::resized,
          [this](int w, int h)
  {
    this->resetContents(w, h);
  }
  );
}




void QResultElement::resetContents(int, int)
{
  if (resultElement_)
  {
    if (!resultElement_->shortDescription().empty())
    {
      shortDesc_->setHtml( QString::fromStdString(
          SimpleLatex(resultElement_->shortDescription()).toHTML(shortDesc_->width())
      ));
    }
    if (!resultElement_->longDescription().empty())
    {
      longDesc_->setHtml( QString::fromStdString(
          SimpleLatex(resultElement_->longDescription()).toHTML(shortDesc_->width())
      ));
    }
  }
}









defineType(QRootResultElement);




QRootResultElement::QRootResultElement(QObject *parent, const QString &label)
  : QResultElement(parent, label, insight::ResultElementPtr())
{}




QVariant QRootResultElement::previewInformation(int) const
{
  return QVariant();
}




void QRootResultElement::createFullDisplay(QVBoxLayout*)
{}






defineType(QStaticTextResultElement);




QStaticTextResultElement::QStaticTextResultElement
(
    QObject *parent,
    const QString &label,
    const QString &staticText,
    const QString& staticDetailText
)
  : QResultElement(parent, label, insight::ResultElementPtr()),
    staticText_(staticText),
    staticDetailText_(staticDetailText)
{}




QVariant QStaticTextResultElement::previewInformation(int role) const
{
  if (role==Qt::DisplayRole) return staticText_;

  return QVariant();
}




void QStaticTextResultElement::createFullDisplay(QVBoxLayout* layout)
{
  layout->addWidget(new QLabel(label_));
  layout->addWidget(new QLabel(staticDetailText_));
}








void QResultSetModel::addResultElements(const ResultElementCollection &rec, QResultElement *parent)
{

  // sort according to stored "order" field
  std::vector<std::pair<ResultElementCollection::key_type,ResultElementCollection::mapped_type> > sortedrec;
  std::copy(rec.begin(), rec.end(), back_inserter(sortedrec));
  std::sort
  (
      sortedrec.begin(), sortedrec.end(),
      [] ( const ResultElementCollection::value_type &left, const ResultElementCollection::value_type &right ) {
            return left.second->order() < right.second->order();
        }
  );

  for ( const auto& re: sortedrec )
  {
      QResultElement *curnode;
      QString label=QString::fromStdString(re.first);

      try
      {
        curnode = QResultElement::lookup
            (
              re.second->type(),
              parent,
              label,
              re.second
            );
      }
      catch (const std::exception& e) {
        curnode=new QStaticTextResultElement(parent, label, "(unknown)", QString::fromStdString(e.what()));
      }

      parent->children_.append(curnode);

      if (const auto *sec = dynamic_cast<ResultElementCollection*>(re.second.get()))
      {
          addResultElements(*sec, curnode);
      }
  }
}




QResultSetModel::QResultSetModel(ResultSetPtr resultSet, QObject* parent)
  : QAbstractItemModel(parent)
{
    root_=new QRootResultElement(this, QString::fromStdString(resultSet->title()));
    addResultElements(*resultSet, root_);
}




QModelIndex QResultSetModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid())
  {
    if (row==0)
    {
      return createIndex( row, column, root_ );
    }
  }
  else
  {
    auto *e=static_cast<QResultElement*>(parent.internalPointer());

    if (row>=0 && row < e->children_.size())
    {
      return createIndex( row, column, e->children_.at(row) );
    }
  }

  return QModelIndex();
}

QModelIndex QResultSetModel::parent(const QModelIndex &index) const
{
  auto *e=static_cast<QResultElement*>(index.internalPointer());

  if (auto pe=e->parentResultElement())
  {
    if (auto ppe=pe->parentResultElement())
    {
      return createIndex(ppe->children_.indexOf(pe), 0, pe);
    }
    else
    {
      return createIndex(0, 0, root_);
    }
  }
  return QModelIndex();
}

int QResultSetModel::columnCount(const QModelIndex &) const
{
  return 2;
}

int QResultSetModel::rowCount(const QModelIndex &parent) const
{
  if (auto *e=static_cast<QResultElement*>(parent.internalPointer()))
  {
    return e->children_.size();
  }

  return 1; // root
}

QVariant QResultSetModel::headerData(int section, Qt::Orientation orient, int role) const
{
  if (role==Qt::DisplayRole)
  {
    if (orient==Qt::Horizontal)
    {
      switch(section)
      {
        case 0: return QVariant("Result element");
        case 1: return QVariant("Summary");
      }
    }
  }
  return QVariant();
}

QVariant QResultSetModel::data(const QModelIndex &index, int role) const
{
  if (auto *e=dynamic_cast<QResultElement*>(static_cast<QObject*>(index.internalPointer())))
  {
    if (index.column()==0)
    {
      if (role==Qt::DisplayRole)
      {
        return QVariant(e->label_);
      }
    }
    else if (index.column()==1)
    {
      return e->previewInformation(role);
    }
  }


  return QVariant();
}


void connectToCWithContentsDisplay(QTreeView* ToCView, QWidget* contentDisplayWidget)
{

  QObject::connect(ToCView, &QTreeView::clicked,
          [contentDisplayWidget](const QModelIndex& i)
  {
    if (auto re = dynamic_cast<insight::QResultElement*>(static_cast<QObject*>(i.internalPointer())))
    {
      qDeleteAll( contentDisplayWidget->children() );

      QVBoxLayout* layout=new QVBoxLayout(contentDisplayWidget);
      contentDisplayWidget->setLayout(layout);

      re->createFullDisplay(layout);

      layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

      auto r=contentDisplayWidget->size();
      re->resetContents(r.width(), r.height());

    }
  }
  );
}


}
