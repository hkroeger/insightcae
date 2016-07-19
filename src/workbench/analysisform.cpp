/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
//  *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef Q_MOC_RUN
#include "base/resultset.h"
#endif

#include "analysisform.h"
#include "ui_analysisform.h"
#include "parameterwrapper.h"
#include "resultelementwrapper.h"

#include <QMessageBox>
#include <QFileDialog>

int metaid1=qRegisterMetaType<insight::ParameterSet>("insight::ParameterSet");
int metaid2=qRegisterMetaType<insight::ResultSetPtr>("insight::ResultSetPtr");


AnalysisWorker::AnalysisWorker(const boost::shared_ptr<insight::Analysis>& analysis)
: analysis_(analysis)
{}

void AnalysisWorker::doWork(insight::ProgressDisplayer* pd)
{
  emit resultReady( (*analysis_)(pd) );
}

AnalysisForm::AnalysisForm(QWidget* parent, const std::string& analysisName)
: QMdiSubWindow(parent)
{
  /*
  insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_.find(analysisName);
  if (i==insight::Analysis::factories_.end())
    throw insight::Exception("Could not lookup analysis type "+analysisName);
  
  analysis_.reset( (*i->second)( insight::NoParameters() ) );*/
  analysis_.reset ( insight::Analysis::lookup(analysisName, insight::NoParameters()) );
  analysis_->setDefaults();
  parameters_ = analysis_->defaultParameters();
  
  ui = new Ui::AnalysisForm;
  QWidget* iw=new QWidget(this);
  ui->setupUi(iw);
  setWidget(iw);
  
  progdisp_=new GraphProgressDisplayer(ui->runTab);
  ui->runTabLayout->addWidget(progdisp_);
  
  this->setWindowTitle(analysis_->getName().c_str());
  connect(ui->runBtn, SIGNAL(clicked()), this, SLOT(onRunAnalysis()));
  connect(ui->killBtn, SIGNAL(clicked()), this, SLOT(onKillAnalysis()));

  connect(ui->saveParamBtn, SIGNAL(clicked()), this, SLOT(onSaveParameters()));
  connect(ui->loadParamBtn, SIGNAL(clicked()), this, SLOT(onLoadParameters()));

  connect(ui->createReportBtn, SIGNAL(clicked()), this, SLOT(onCreateReport()));
  

//   addWrapperToWidget(parameters_, ui->inputContents, this);
  QTreeWidgetItem* root=new QTreeWidgetItem(0);
  root->setText(0, "Parameters");
  ui->ptree->setColumnCount(2);
  ui->ptree->setHeaderLabels( QStringList() << "Parameter Name" << "Current Value" );
  ui->ptree->addTopLevelItem(root);

  DirectoryParameterWrapper *dp = 
     new DirectoryParameterWrapper
     ( 
      ParameterWrapper::ConstrP
      (
        root, 
        "execution directory", 
        analysis_->executionPathParameter(),
        ui->inputContents, 
	this
      ) 
    );
  QObject::connect(ui->ptree, SIGNAL(itemSelectionChanged()),
	  dp, SLOT(onSelectionChanged()));
  QObject::connect(this, SIGNAL(apply()), dp, SLOT(onApply()));
  QObject::connect(this, SIGNAL(update()), dp, SLOT(onUpdate()));
  
  addWrapperToWidget(parameters_, root, ui->inputContents, this);
  
  ui->ptree->expandAll();
  ui->ptree->resizeColumnToContents(0);
  ui->ptree->resizeColumnToContents(1);
  ui->ptree->setContextMenuPolicy(Qt::CustomContextMenu);
}

AnalysisForm::~AnalysisForm()
{
  workerThread_.quit();
  workerThread_.wait();
  delete ui;
}

void AnalysisForm::onSaveParameters()
{
//   emit apply();

  QString fn = QFileDialog::getSaveFileName(this, "Save Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty())
  {
    parameters_.saveToFile(fn.toStdString(), analysis_->type());
  }
}

void AnalysisForm::onLoadParameters()
{
  QString fn = QFileDialog::getOpenFileName(this, "Open Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty())
  {
    parameters_.readFromFile(fn.toStdString());
    emit update();
  }
}

void AnalysisForm::onRunAnalysis()
{
  if (!workerThread_.isRunning())
  {
    emit apply();
    analysis_->setParameters(parameters_);
    
    progdisp_->reset();
    
    AnalysisWorker *worker = new AnalysisWorker(analysis_);
    worker->moveToThread(&workerThread_);
    connect(&workerThread_, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(this, SIGNAL(runAnalysis(insight::ProgressDisplayer*)), 
	    worker, SLOT(doWork(insight::ProgressDisplayer*)));
    connect(worker, SIGNAL(resultReady(insight::ResultSetPtr)), this, SLOT(onResultReady(insight::ResultSetPtr)));
    workerThread_.start();

    ui->tabWidget->setCurrentWidget(ui->runTab);

    emit runAnalysis(progdisp_);
  }
}

void AnalysisForm::onKillAnalysis()
{
  if (workerThread_.isRunning())
  {
    analysis_->cancel();
    workerThread_.quit();
    workerThread_.wait();
  }
}

void AnalysisForm::onResultReady(insight::ResultSetPtr results)
{
  results_=results;
  
  qDeleteAll(ui->outputContents->findChildren<ResultElementWrapper*>());
  addWrapperToWidget(*results_, ui->outputContents, this);

  ui->tabWidget->setCurrentWidget(ui->outputTab);

  QMessageBox::information(this, "Finished!", "The analysis has finished");
}

void AnalysisForm::onCreateReport()
{
  if (!results_.get())
  {
    QMessageBox::critical(this, "Error", "No results present!");
    return;
  }
  
  QString fn = QFileDialog::getSaveFileName
  (
      this, 
    "Save Latex Report", 
    QString(analysis_->executionPath().c_str()), 
    "LaTeX file (*.tex)"
  );
  if (!fn.isEmpty())
  {
    boost::filesystem::path outpath=fn.toStdString();
    results_->writeLatexFile( outpath );

    QMessageBox::information(this, "Done!", QString("The report has been created as\n")+outpath.c_str());
  }


}


