#include "iqresultsetmodel.h"


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






defineType(IQResultElement);
defineFactoryTable(
    IQResultElement,
    LIST(QObject* parent, const QString& label, insight::ResultElementPtr rep),
    LIST(parent, label, rep)
);




IQResultElement::IQResultElement(QObject *parent, const QString& label, insight::ResultElementPtr rep)
    : QObject(parent),
      resultElement_(rep),
      label_(label),
      checkState_(Qt::Checked)
{}




IQResultElement *IQResultElement::parentResultElement() const
{
  return dynamic_cast<IQResultElement*>(parent());
}




ResultElement *IQResultElement::resultElement() const
{
  return resultElement_.get();
}




void IQResultElement::createFullDisplay(QVBoxLayout* layout)
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




void IQResultElement::resetContents(int, int)
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



Qt::CheckState IQResultElement::isChecked() const
{
    return checkState_;
}

void IQResultElement::setChecked( Qt::CheckState cs )
{
    checkState_=cs;
}





defineType(IQRootResultElement);




IQRootResultElement::IQRootResultElement(QObject *parent, const QString &label)
  : IQResultElement(parent, label, insight::ResultElementPtr())
{}




QVariant IQRootResultElement::previewInformation(int) const
{
  return QVariant();
}




void IQRootResultElement::createFullDisplay(QVBoxLayout*)
{}






defineType(IQStaticTextResultElement);




IQStaticTextResultElement::IQStaticTextResultElement
(
    QObject *parent,
    const QString &label,
    const QString &staticText,
    const QString& staticDetailText
)
  : IQResultElement(parent, label, insight::ResultElementPtr()),
    staticText_(staticText),
    staticDetailText_(staticDetailText)
{}




QVariant IQStaticTextResultElement::previewInformation(int role) const
{
  if (role==Qt::DisplayRole) return staticText_;

  return QVariant();
}




void IQStaticTextResultElement::createFullDisplay(QVBoxLayout* layout)
{
  layout->addWidget(new QLabel(label_));
  layout->addWidget(new QLabel(staticDetailText_));
}








void IQResultSetModel::addResultElements(const ResultElementCollection &rec, IQResultElement *parent)
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
      IQResultElement *curnode;
      QString label=QString::fromStdString(re.first);

      try
      {
        curnode = IQResultElement::lookup
            (
              re.second->type(),
              parent,
              label,
              re.second
            );
      }
      catch (const std::exception& e) {
        curnode=new IQStaticTextResultElement(
              parent, label,
              "",
              "(There is currently no viewer implemented for this result element.)"
              //QString::fromStdString(e.what())
              );
      }

      parent->children_.append(curnode);

      if (const auto *sec = dynamic_cast<ResultElementCollection*>(re.second.get()))
      {
          addResultElements(*sec, curnode);
      }
  }
}




void IQResultSetModel::setChildrenCheckstate(const QModelIndex& idx, bool checked)
{
    for (int row=0; row<rowCount(idx); ++row)
    {
        auto cidx=index(row, 0, idx);
        if (auto *c = dynamic_cast<IQResultElement*>(
                    static_cast<QObject*>( cidx.internalPointer())))
        {
            c->setChecked( checked?Qt::Checked:Qt::Unchecked );
            emit dataChanged(cidx, cidx);
            setChildrenCheckstate(cidx, checked);
        }
    }
}




bool IQResultSetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
     if (role == Qt::CheckStateRole)
     {
         if (auto *e = dynamic_cast<IQResultElement*>(
                     static_cast<QObject*>(index.internalPointer())))
         {
             e->setChecked( value.toBool()?Qt::Checked:Qt::Unchecked );
             emit dataChanged(index, index);
             setChildrenCheckstate( index, value.toBool() );
             updateParentCheckState( index );
             return true;
         }
     }

     return false;
}




IQResultSetModel::IQResultSetModel(ResultSetPtr resultSet, bool selectableElements, QObject* parent)
  : QAbstractItemModel(parent),
    selectableElements_(selectableElements)
{
    root_=new IQRootResultElement(this, QString::fromStdString(resultSet->title()));
    addResultElements(*resultSet, root_);
}




QModelIndex IQResultSetModel::index(int row, int column, const QModelIndex &parent) const
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
    auto *e=static_cast<IQResultElement*>(parent.internalPointer());

    if (row>=0 && row < e->children_.size())
    {
      return createIndex( row, column, e->children_.at(row) );
    }
  }

  return QModelIndex();
}




QModelIndex IQResultSetModel::parent(const QModelIndex &index) const
{
  auto *e=static_cast<IQResultElement*>(index.internalPointer());

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




int IQResultSetModel::columnCount(const QModelIndex &) const
{
  return 2 + (selectableElements_? 1 : 0);
}




int IQResultSetModel::rowCount(const QModelIndex &parent) const
{
  if (auto *e=static_cast<IQResultElement*>(parent.internalPointer()))
  {
    return e->children_.size();
  }

  return 1; // root
}

QVariant IQResultSetModel::headerData(int section, Qt::Orientation orient, int role) const
{
  if (role==Qt::DisplayRole)
  {
    if (orient==Qt::Horizontal)
    {
      int dc0=selectableElements_?1:0;
      if (section==0 && selectableElements_) return QVariant("Select");
      else if (section == dc0+0) return QVariant("Result element");
      else if (section == dc0+1) return QVariant("Summary");
    }
  }
  return QVariant();
}




QVariant IQResultSetModel::data(const QModelIndex &index, int role) const
{
  if (auto *e=dynamic_cast<IQResultElement*>(static_cast<QObject*>(index.internalPointer())))
  {
      if (role == Qt::CheckStateRole)
      {
          if ( index.column()==0 && selectableElements_)
              return QVariant(e->isChecked());
      }
      if (role==Qt::DisplayRole)
      {
          int dc0=selectableElements_?1:0;
          if (index.column()==dc0+0) return QVariant(e->label_);
          else if (index.column()==dc0+1) return e->previewInformation(role);
      }
  }


  return QVariant();
}




Qt::ItemFlags IQResultSetModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if ( index.column() == 0 && selectableElements_ )
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

void IQResultSetModel::updateParentCheckState(const QModelIndex &idx)
{
    auto pidx=parent(idx);
    if (pidx.isValid())
    {
        if (auto *e = dynamic_cast<IQResultElement*>(
                    static_cast<QObject*>( pidx.internalPointer())))
        {
            int nChecked=0, nUnchecked=0, nPartChecked=0;
            for (int row=0; row<rowCount(pidx); ++row)
            {
                auto cidx=index(row, 0, pidx);
                if (auto *c = dynamic_cast<IQResultElement*>(
                            static_cast<QObject*>( cidx.internalPointer())))
                {
                    switch(c->isChecked())
                    {
                    case Qt::Checked: nChecked++; break;
                    case Qt::Unchecked: nUnchecked++; break;
                    case Qt::PartiallyChecked: nPartChecked++; break;
                    }
                }
            }

            auto oldcs=e->isChecked();
            Qt::CheckState newcs=oldcs;
            if (nChecked>0 && nUnchecked==0 && nPartChecked==0)
                newcs=Qt::Checked;
            else if (nUnchecked>0 && nChecked==0 && nPartChecked==0)
                newcs=Qt::Unchecked;
            else
                newcs=Qt::PartiallyChecked;
            insight::dbg()<<oldcs<<" "<<nChecked<<" "<<nUnchecked<<" "<<nPartChecked<<" "<<newcs<<std::endl;
            if (newcs!=oldcs)
            {
                e->setChecked(newcs);
                emit dataChanged(pidx, pidx);
                updateParentCheckState(pidx);
            }
        }
    }
}




void connectToCWithContentsDisplay(QTreeView* ToCView, QWidget* contentDisplayWidget)
{

  QObject::connect(ToCView, &QTreeView::clicked,
          [contentDisplayWidget](const QModelIndex& i)
  {
    if (auto re = dynamic_cast<insight::IQResultElement*>(static_cast<QObject*>(i.internalPointer())))
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
