#ifndef IQRESULTSETDISPLAYERWIDGET_H
#define IQRESULTSETDISPLAYERWIDGET_H

#include <QWidget>

#include "iqfilteredresultsetmodel.h"
#include "iqresultsetfiltermodel.h"

namespace Ui {
class IQResultSetDisplayerWidget;
}

class IQResultSetDisplayerWidget : public QWidget
{
    Q_OBJECT


    insight::IQResultSetModel *resultsModel_;
    insight::IQFilteredResultSetModel *filteredResultsModel_;
    IQResultSetFilterModel *filterModel_;

public:
    explicit IQResultSetDisplayerWidget(QWidget *parent = nullptr);
    ~IQResultSetDisplayerWidget();

    void clear();
    void loadResults(std::unique_ptr<insight::ResultSet> results);

    bool hasResults() const;

public Q_SLOTS:
    void loadResultSet();
    void saveResultSetAs();
    void renderReport(insight::ProgressDisplayer *pd = nullptr);
    void loadFilter();
    void saveFilter();

private:
    Ui::IQResultSetDisplayerWidget *ui;
};

#endif // IQRESULTSETDISPLAYERWIDGET_H
