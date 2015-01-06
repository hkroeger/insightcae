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

#ifndef workbench_H
#define workbench_H

#include <QtGui/QMainWindow>
#include <QtGui/QMdiArea>

#include <QApplication>

class WorkbenchApplication
: public QApplication
{
  Q_OBJECT

public:

  WorkbenchApplication( int &argc, char **argv);
  ~WorkbenchApplication( );

  bool notify(QObject *rec, QEvent *ev);
  
signals:
  void exceptionOcurred(QString msg, QString addinfo);
  
public slots:
  void displayExceptionNotification(QString msg, QString addinfo);

};


class workbench
: public QMainWindow
{
Q_OBJECT
private:
  QMdiArea *mdiArea_;
  
public:
    workbench();
    virtual ~workbench();

    void openAnalysis(const QString& fn);
    
private slots:
    void newAnalysis();
    void onOpenAnalysis();
};

#endif // workbench_H
