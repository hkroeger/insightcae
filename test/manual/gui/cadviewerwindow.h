#ifndef CADVIEWERWINDOW_H
#define CADVIEWERWINDOW_H

#include <QMainWindow>
#include <QTreeView>

#include "iqcaditemmodel.h"
#include "iqvtkcadmodel3dviewer.h"

class CADViewerWindow
        : public QMainWindow
{
    IQCADItemModel itemModel_;
    IQVTKCADModel3DViewer *viewer_;
    QTreeView *treeView_;

public:
    CADViewerWindow();

    QSize sizeHint() const override;
};

#endif // CADVIEWERWINDOW_H
