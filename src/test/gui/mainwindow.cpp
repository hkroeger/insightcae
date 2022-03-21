
//#ifdef HAVE_WT
//#include "remoterun.h"
//#endif
//#include "localrun.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#endif

#include "base/analysis.h"
#include "base/remoteserverlist.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "parameterwrapper.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QScrollBar>
#include <QStatusBar>
#include <QSettings>
#include <QProcess>
#include "email.h"


#include <cstdlib>
#include <memory>

#include "of_clean_case.h"
#include "remotesync.h"
#include "base/remoteserverlist.h"
#include "remotedirselector.h"
#include "iqremoteparaviewdialog.h"

#include "qoccviewwidget.h"

#include "numericalwindtunnel.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

   auto defaultParams = insight::Analysis::defaultParameters("Numerical Wind Tunnel");
   parameters_ = defaultParams;
  insight::AnalysisPtr a( insight::Analysis::lookup("Numerical Wind Tunnel", defaultParams, "") );
//  insight::AnalysisPtr a( new insight::GasDispersion(defaultParams, "") );

  setCentralWidget(new QoccViewWidget(this));
}

MainWindow::~MainWindow()
{
  delete ui;
}
