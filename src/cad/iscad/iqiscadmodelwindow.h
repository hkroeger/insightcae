#ifndef IQISCADMODELWINDOW_H
#define IQISCADMODELWINDOW_H

#include <QWidget>
#include <QTreeView>
#include <QTextEdit>

namespace boost { namespace filesystem { class path; } }
class IQCADItemModel;
class IQVTKCADModel3DViewer;
class IQISCADModelScriptEdit;

/**
 * a widget, which arranges all widgets, required for editing a model:
 * text editor, graphical window, model tree and buttons
 */
class IQISCADModelWindow
: public QWidget
{
    Q_OBJECT

public:
    typedef IQVTKCADModel3DViewer Model3DViewer;

protected:
    IQCADItemModel* model_;
    IQISCADModelScriptEdit* modelEdit_;
    QTreeView* modelTree_;
    Model3DViewer* viewer_;
    QTextEdit* notepad_;

public:
    IQISCADModelWindow(QWidget* parent = 0);

    IQCADItemModel* model();
    IQISCADModelScriptEdit* modelEdit();
    QTreeView* modelTree();
    Model3DViewer* viewer();
    QTextEdit* notepad();

public Q_SLOTS:
    void onCopyBtnClicked();
    void onUpdateTitle(const boost::filesystem::path& filepath, bool isUnSaved);
    void onInsertNotebookText(const QString& text);
    void viewerSettings();

protected:
    virtual void closeEvent(QCloseEvent *event);

signals:
    void updateTabTitle(IQISCADModelWindow* model, const boost::filesystem::path& filepath, bool isUnSaved);
};


#endif // IQISCADMODELWINDOW_H
