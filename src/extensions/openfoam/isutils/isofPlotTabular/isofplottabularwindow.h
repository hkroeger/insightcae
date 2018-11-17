#ifndef ISOFPLOTTABULARWINDOW_H
#define ISOFPLOTTABULARWINDOW_H

#include <QtGui>
#include "ui_isofplottabularwindow.h"

#include "base/boost_include.h"
#include "base/linearalgebra.h"

#include <vector>

#include "qwt_plot_curve.h"

class IsofPlotTabularWindow : public QMainWindow
{
    Q_OBJECT

    Ui_MainWindow *ui;

    boost::filesystem::path file_;
    arma::mat data_;

    std::vector<QwtPlotCurve*> curve_;

public:
    IsofPlotTabularWindow(const boost::filesystem::path& file);

public Q_SLOTS:
    void onUpdate();
    void onTabChanged(int);
};

#endif // ISOFPLOTTABULARWINDOW_H
