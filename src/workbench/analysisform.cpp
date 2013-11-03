/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "analysisform.h"
#include "ui_analysisform.h"
#include "parameterwrapper.h"



AnalysisForm::AnalysisForm(QWidget* parent, const std::string& analysisName)
: QMdiSubWindow(parent)
{
  
  insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_.find(analysisName);
  if (i==insight::Analysis::factories_.end())
    throw insight::Exception("Could not lookup analysis type "+analysisName);
  
  analysis_.reset( (*i->second)( insight::NoParameters() ) );
  parameters_ = analysis_->defaultParameters();
  
  ui = new Ui::AnalysisForm;
  QWidget* iw=new QWidget(this);
  ui->setupUi(iw);
  setWidget(iw);
  this->setWindowTitle(analysis_->getName().c_str());
  connect(ui->runBtn, SIGNAL(clicked()), this, SLOT(runAnalysis()));
  
  addWrapperToWidget(parameters_, ui->inputContents, this);
      
}

AnalysisForm::~AnalysisForm()
{
    delete ui;
}

void AnalysisForm::runAnalysis()
{
  emit apply();
  (*analysis_)(parameters_);
}

