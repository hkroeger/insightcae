
#include <QApplication>
#include <QDebug>

#include "qactionprogressdisplayerwidget.h"

#include "boost/algorithm/string.hpp"

#include <iostream>
#include <cmath>

using namespace std;
using namespace boost;

namespace insight {


QActionProgressDisplayerWidget::Columns::iterator QActionProgressDisplayerWidget::getColumn(const string &path, std::vector<std::string>& splitPath)
{
  string colname;
  std::vector<std::string> variant_path;
  split(variant_path, path, is_any_of(":"));
  if (variant_path.size()==2)
  {
    colname=variant_path[0];
    split(splitPath, variant_path[1], is_any_of("/"));
  }
  else if (variant_path.size()==1)
  {
    colname="";
    split(splitPath, variant_path[0], is_any_of("/"));
  }

  auto ic = columns_.find(colname);
  if (ic==columns_.end())
  {
    Rows nc;
    nc.vlayout=new QVBoxLayout;
    nc.lbl=new QLabel("<b>"+QString::fromStdString(colname)+"</b>");
    nc.vlayout->addWidget(nc.lbl);
    hlayout_->addLayout(nc.vlayout);
    columns_[colname]=nc;
    ic=columns_.find(colname);
  }


  return ic;
}

QActionProgressDisplayerWidget::ProgressItem
QActionProgressDisplayerWidget::getOrCreateItem
(const std::string &path)
{
  vector<string> splitPath;
  Rows *c = &(getColumn(path, splitPath)->second);

  for (auto& r: splitPath)
  {
    auto ir=c->items.find(r);
    if (ir==c->items.end())
    {
      qDebug()<<"adding progress bar "<<QString::fromStdString(path);
      ProgressItem pi;

      pi.p=new QProgressBar;
      pi.p->setMinimum(0);
      pi.p->setMaximum(100);
      pi.p->setTextVisible(true);
      pi.p->setAlignment(Qt::AlignCenter);
      pi.lbl=new QLabel(QString::fromStdString(r));

      c->vlayout->addWidget(pi.lbl);
      c->vlayout->addWidget(pi.p);
      c->items[r]=pi;
    }
  }

  return c->items.at( splitPath.back() );
}

void QActionProgressDisplayerWidget::deleteItem(const string &path)
{
  qDebug()<<"deleting progress bar "<<QString::fromStdString(path);

  vector<string> splitPath;
  auto ic=getColumn(path, splitPath);

  if (ic!=columns_.end())
  {
    Rows *c=&(ic->second);

    auto ir=c->items.find(splitPath.back());
    if (ir!=c->items.end())
    {
      ir->second.deleteLater();
      c->items.erase(ir);
    }

    if (c->items.size()==0)
    {
      c->deleteLater();
      columns_.erase(ic);
    }
  }

}

QActionProgressDisplayerWidget::QActionProgressDisplayerWidget
(QWidget *parent)
  : QWidget(parent)
{
  hlayout_=new QHBoxLayout;
  setLayout(hlayout_);
}


void QActionProgressDisplayerWidget::setActionProgressValue(const std::string &path, double value)
{
  QMetaObject::invokeMethod(
        qApp,
        [this,path,value]()
        {
          qDebug()<<"setting value for "<<QString::fromStdString(path)<<" to "<<100.*value;
          auto i=getOrCreateItem(path);
          i.p->setValue(std::round(100.*value));
        },
        Qt::QueuedConnection
  );
}

void QActionProgressDisplayerWidget::setMessageText(const std::string &path, const std::string &message)
{
  QMetaObject::invokeMethod(
        qApp,
        [this,path,message]()
        {
          qDebug()<<"setting text for "<<QString::fromStdString(path)<<" to "<<QString::fromStdString(message);
          auto i=getOrCreateItem(path);
          i.p->setFormat( QString::fromStdString(message) );
        },
        Qt::QueuedConnection
  );
}

void QActionProgressDisplayerWidget::finishActionProgress(const string &path)
{
  QMetaObject::invokeMethod(
        qApp,
        [this,path]()
        {
          deleteItem(path);
        },
        Qt::QueuedConnection
  );
}


void QActionProgressDisplayerWidget::reset()
{
  for (auto& c: columns_)
    c.second.deleteLater();
  columns_.clear();
}


void QActionProgressDisplayerWidget::update(const ProgressState &/*pi*/)
{}

} // namespace insight
