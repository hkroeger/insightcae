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


#ifndef IQGRAPHPROGRESSDISPLAYER_H
#define IQGRAPHPROGRESSDISPLAYER_H

#include "toolkit_gui_export.h"


#ifndef Q_MOC_RUN
#include <base/analysis.h>
#endif

#include "iqgraphprogresschart.h"

#include <map>
#include <vector>

#include <QWidget>
#include <QLabel>

#include <QTabWidget>


class TOOLKIT_GUI_EXPORT IQGraphProgressDisplayer
: public QTabWidget,
  public insight::ProgressDisplayer
{
  Q_OBJECT
  
protected:
  std::map<std::string, IQGraphProgressChart*> charts_;

  void createChart(bool log, const std::string name);

public:
  IQGraphProgressDisplayer(QWidget* parent=nullptr);
  virtual ~IQGraphProgressDisplayer();

  IQGraphProgressChart* addChartIfNeeded(const std::string& name);

  void reset() override;

  void update(const insight::ProgressState& pi) override;
  void logMessage(const std::string& line) override;
  void setActionProgressValue(const std::string& path, double value) override;
  void setMessageText(const std::string& path, const std::string& message) override;
  void finishActionProgress(const std::string& path) override;

};




#endif // IQGRAPHPROGRESSDISPLAYER_H
