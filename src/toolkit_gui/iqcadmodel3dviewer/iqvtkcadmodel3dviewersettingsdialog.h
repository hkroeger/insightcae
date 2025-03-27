#ifndef IQVTKCADMODEL3DVIEWERSETTINGSDIALOG_H
#define IQVTKCADMODEL3DVIEWERSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class IQVTKCADModel3DViewerSettingsDialog;
}

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IQVTKCADModel3DViewerSettingsDialog(IQVTKCADModel3DViewer* viewer, QWidget *parent = nullptr);
    ~IQVTKCADModel3DViewerSettingsDialog();

    void accept() override;

public Q_SLOTS:
    void apply();

private:
    Ui::IQVTKCADModel3DViewerSettingsDialog *ui;
    IQVTKCADModel3DViewer* viewer_;
};

#endif // IQVTKCADMODEL3DVIEWERSETTINGSDIALOG_H
