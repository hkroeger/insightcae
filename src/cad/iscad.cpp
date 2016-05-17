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

#include <QtGui/QApplication>
#include "base/boost_include.h"

#include "base/exception.h"
#include "iscadapplication.h"

#include <QMainWindow>

#include "parser.h"

#include <locale>
#include <QLocale>

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
  if (argc==3) 
  {
    if (std::string(argv[1])!="-batch")
    {
      cout<<"Invalid command line!"<<endl;
      exit(-1);
    }
    insight::cad::ModelPtr model(new insight::cad::Model);
    return insight::cad::parseISCADModelFile(argv[2], model.get());
  }
  else
  {
    ISCADApplication app(argc, argv);
    std::locale::global(std::locale::classic());
    QLocale::setDefault(QLocale::C);
    
    QPixmap pixmap(":/resources/insight_cad_splash.png");
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint|Qt::SplashScreen);
    splash.show();
    splash.showMessage(/*propGeoVersion()+" - */"Wait...");

    ISCADMainWindow window;
    if (argc==2) 
      window.loadFile(argv[1]);

    window.show();

    app.processEvents();//This is used to accept a click on the screen so that user can cancel the screen

    I w(&splash, &window);
    w.start(); // splash is shown for 5 seconds

//     splash.finish(&window);
    window.raise();

    return app.exec();
  }
}
