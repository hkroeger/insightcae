#ifndef NEWANALYSISFORM_H
#define NEWANALYSISFORM_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>

namespace Ui {
class NewAnalysisForm;
}

class NewAnalysisForm : public QWidget
{
    Q_OBJECT


    void fillAnalysisList(QTreeWidget* treeWidget);

public:
    explicit NewAnalysisForm(QWidget *parent = nullptr);
    ~NewAnalysisForm();

    void replaceLoadButton(QPushButton *b);

private:
    Ui::NewAnalysisForm *ui;

Q_SIGNALS:
    void createAnalysis(const std::string& name);
    void openAnalysis();
};

#endif // NEWANALYSISFORM_H
