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

#ifndef ISOFPLOTTABULARWINDOW_H
#define ISOFPLOTTABULARWINDOW_H

#include "base/boost_include.h"
#include "base/linearalgebra.h"

#include <QtGui>
#include "ui_isofplottabularwindow.h"


#include <vector>

#include <QtCharts/QLineSeries>

class IsofPlotTabularWindow : public QMainWindow
{
    Q_OBJECT

    Ui_MainWindow *ui;

    std::vector<boost::filesystem::path> files_;
    arma::mat data_;

    std::vector<QtCharts::QLineSeries*> curve_;

    void readFiles();

    mutable std::set<int> completedAverage_;

public:
    IsofPlotTabularWindow(const std::vector<boost::filesystem::path>& files);

    void resetAllAvgFractions(double f);

public Q_SLOTS:
    void onUpdate(bool checked=false);
    void onSaveFinalValues(QString outFile = QString());
    void onTabChanged(int);

    void averageValueReady(int i);

Q_SIGNALS:
    void allAverageValuesReady();
};

#endif // ISOFPLOTTABULARWINDOW_H
