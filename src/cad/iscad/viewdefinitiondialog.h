#ifndef VIEWDEFINITIONDIALOG_H
#define VIEWDEFINITIONDIALOG_H

#include <QDialog>
#include <set>

namespace Ui {
class ViewDefinitionDialog;
}

enum AddedView { l, r, t, b, k };
struct DrawingViewDefinition
{
    std::string label = "iso";
    std::string onPointExpr="O", normalExpr="EX+EY+EZ", upExpr="EY";
    bool isSection=false, poly=false, skipHL=false;
    std::set<AddedView> add;
};

class ViewDefinitionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewDefinitionDialog(
        QWidget *parent = nullptr,
        const DrawingViewDefinition& vd = DrawingViewDefinition());
    ~ViewDefinitionDialog();

    void accept() override;

    const DrawingViewDefinition& viewDef() const;

private:
    DrawingViewDefinition viewDef_;
    Ui::ViewDefinitionDialog *ui;
};

#endif // VIEWDEFINITIONDIALOG_H
