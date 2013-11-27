/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  Hannes Kroeger <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "resultelementwrapper.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QFileDialog>
#include <QInputDialog>

#include "boost/foreach.hpp"

using namespace boost;

defineType(ResultElementWrapper);
defineFactoryTable(ResultElementWrapper, ResultElementWrapper::ConstrP);

ResultElementWrapper::ResultElementWrapper(const ConstrP& p)
: QWidget(get<0>(p)),
  name_(get<1>(p)),
  p_(get<2>(p))
{}

ResultElementWrapper::~ResultElementWrapper()
{}


defineType(ScalarResultWrapper);
addToFactoryTable(ResultElementWrapper, ScalarResultWrapper, ResultElementWrapper::ConstrP);

ScalarResultWrapper::ScalarResultWrapper(const ConstrP& p)
: ResultElementWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLabel(this);
  le_->setText(QString::number(res().value()) + " " + res().unit().c_str() );
  le_->setToolTip(QString(res().shortDescription().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}

defineType(ImageWrapper);
addToFactoryTable(ResultElementWrapper, ImageWrapper, ResultElementWrapper::ConstrP);

ImageWrapper::ImageWrapper(const ConstrP& p)
: ResultElementWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QPixmap image(res().imagePath().c_str());
  
  // scale 300dpi (print) => 70dpi (screen)
  double w0=image.size().width();
  image=image.scaledToWidth(w0/4, Qt::SmoothTransformation);
  
  le_=new QLabel(this);
  le_->setPixmap(image);
  le_->setScaledContents(true);
  
  le_->setToolTip(QString(res().shortDescription().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}


defineType(TabularResultWrapper);
addToFactoryTable(ResultElementWrapper, TabularResultWrapper, ResultElementWrapper::ConstrP);

TabularResultWrapper::TabularResultWrapper(const ConstrP& p)
: ResultElementWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  le_=new QTableWidget(res().rows().size(), res().headings().size(), this);
  
  QStringList headers;
  BOOST_FOREACH(const std::string& h, res().headings() )
  {
    headers << QString(h.c_str());
  }
  le_->setHorizontalHeaderLabels( headers );
  
  for (size_t i=0; i<res().rows().size(); i++)
  {
    const std::vector<double>& row=res().rows()[i];
    for (size_t j=0; j<row.size(); j++)
    {
      le_->setItem(i, j, new QTableWidgetItem( QString::number(row[j]) ));
    }
  }
  
  le_->setToolTip(QString(res().shortDescription().c_str()));
  layout->addWidget(le_);
  
  this->setLayout(layout);
}


void addWrapperToWidget(insight::ResultSet& rset, QWidget *widget, QWidget *superform)
{
  QVBoxLayout *vlayout=new QVBoxLayout(widget);
  for(insight::ResultSet::iterator i=rset.begin(); i!=rset.end(); i++)
      {
	ResultElementWrapper *wrapper = 
	  ResultElementWrapper::lookup
	  (
	    i->second->type(),
	    ResultElementWrapper::ConstrP(widget, i->first.c_str(), *i->second)
	  );
	vlayout->addWidget(wrapper);
	/*
	if (superform) 
	{
	  QObject::connect(superform, SIGNAL(apply()), wrapper, SLOT(onApply()));
	  QObject::connect(superform, SIGNAL(update()), wrapper, SLOT(onUpdate()));
	}
	*/
      }
      
}