#ifndef IQCHARTINTERACTIVELEGEND_H
#define IQCHARTINTERACTIVELEGEND_H

#include "toolkit_gui_export.h"

#include <QWidget>
#include <QHBoxLayout>




class IQLineSeriesData;
class FlowLayout;



class TOOLKIT_GUI_EXPORT IQChartInteractiveLegend
: public QWidget
{
    FlowLayout *layout_;

public:
    IQChartInteractiveLegend(QWidget* parent = nullptr);

    void addEntry(QString name, IQLineSeriesData *ld);
};




#endif // IQCHARTINTERACTIVELEGEND_H
