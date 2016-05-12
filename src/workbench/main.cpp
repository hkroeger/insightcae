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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>
#include <QtGui/QApplication>
#include "base/boost_include.h"
#include "workbench.h"

#include "base/exception.h"
#include "base/linearalgebra.h"

#include <qthread.h>
 
class I : public QThread
{
  QSplashScreen* sp_;
  QWidget win_;
public:
  I(QSplashScreen* sp, QWidget* win) :sp_(sp), win_(win) {}
  
  void run() { QThread::sleep(3); sp_->finish(&win_); }
};

int main(int argc, char** argv)
{
  insight::UnhandledExceptionHandling ueh;
  insight::GSLExceptionHandling gsl_errtreatment;
  
  WorkbenchApplication app(argc, argv);

  // After creation of application object!
  std::locale::global(std::locale::classic());
  QLocale::setDefault(QLocale::C);

  QPixmap pixmap(":/resources/insight_workbench_splash.png");
  QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint|Qt::SplashScreen);
  splash.show();
  splash.showMessage(/*propGeoVersion()+" - */"Wait...");

  workbench window;
  
  if (argc>1)
  {
    boost::filesystem::path fn(argv[1]);
    window.openAnalysis(boost::filesystem::absolute(fn).c_str());
  }
  window.show();
  
  app.processEvents();//This is used to accept a click on the screen so that user can cancel the screen
  
  I w(&splash, &window);
  w.start(); // splash is shown for 5 seconds
  
  window.raise();

  return app.exec();
}
