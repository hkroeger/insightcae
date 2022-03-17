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

#include <QMainWindow>
#include <QMdiArea>

#include <QApplication>
#include <QMenuBar>
#include <QAction>
#include <QThread>
#include <QPointer>

#include "insightcaeapplication.h"
#include "sdmdiarea.h"

#include <array>

#include "base/wsllinuxserver.h"


class WidgetWithDynamicMenuEntries
        : public QObject
{
    Q_OBJECT

    std::vector<QObject*> dynamicGUIElements_;

public:
    WidgetWithDynamicMenuEntries(QObject* parent, const std::vector<QObject*>& dynamicGUIElements = {});
    ~WidgetWithDynamicMenuEntries();

    template<class O>
    O* add(O* obj)
    {
        dynamicGUIElements_.push_back(obj);
        return obj;
    }

};


class workbench
: public QMainWindow
{
Q_OBJECT

private:
  SDMdiArea *mdiArea_;

  QAction *separatorAct_;
  std::array<QAction *,5> recentFileActs_;

  bool logToConsole_;

  void updateRecentFileActions();

public:

   QPointer<WidgetWithDynamicMenuEntries> lastActive_;

public:
    workbench(bool logToConsole=false);
    virtual ~workbench();

    void openAnalysis(const QString& fn);
    void closeEvent(QCloseEvent *event);
    void readSettings();
    void show();

public Q_SLOTS:
    void newAnalysis(std::string analysisType = "");

private Q_SLOTS:
    void onOpenAnalysis();
    void openRecentFile();

    void checkInstallation(bool reportSummary=false);

private slots:
    void onSubWindowActivated( QMdiSubWindow * window );
};

#endif // workbench_H
