#ifndef QTEXTENSIONS_H
#define QTEXTENSIONS_H

#include <set>
#include <type_traits>

#include "base/boost_include.h"
#include "base/latextools.h"

#include <functional>

#include <QObject>
#include <QGridLayout>
#include <QTextEdit>
#include <QLabel>
#include <QResizeEvent>
#include <QMetaObject>
#include <QThread>
#include <QMainWindow>
#include <QCoreApplication>
#include <QModelIndex>
#include <QTreeView>

#include "boost/signals2.hpp"

#include "toolkit_gui_export.h"


QMainWindow* getMainWindow();

/**
 * @brief collapseMatchingNodes
 * Traverse all nodes in the tree view breadth-first and collapse every node
 * for which @p shouldCollapse returns true. Children of a collapsed node are
 * not visited (and therefore stay collapsed too).
 */
TOOLKIT_GUI_EXPORT void collapseMatchingNodes(
    QTreeView* view,
    const std::function<bool(const QModelIndex&)>& shouldCollapse);

/**
 * @brief availableContentHeight
 * Reasonable height budget for a content widget so it doesn't outgrow the
 * space its parent widget currently has (falls back to a fraction of the
 * top-level window if not yet parented).
 */
TOOLKIT_GUI_EXPORT int availableContentHeight(const QWidget* w);

/**
 * @brief runInGUIThread
 * Execute a callable on the GUI thread and return its result to the caller.
 * Safe to call from any thread, including the GUI thread itself.
 * When called from a background thread, the calling thread blocks until the
 * GUI thread has finished executing func (Qt::BlockingQueuedConnection).
 */
template<typename Func>
auto runInGUIThread(Func&& func)
    -> std::enable_if_t<!std::is_void_v<decltype(func())>, decltype(func())>
{
    if (QThread::currentThread() == qApp->thread())
        return func();

    using R = decltype(func());
    R result{};
    QMetaObject::invokeMethod(
        qApp,
        [&result, f=std::forward<Func>(func)]() mutable { result = f(); },
        Qt::BlockingQueuedConnection
    );
    return result;
}

template<typename Func>
auto runInGUIThread(Func&& func)
    -> std::enable_if_t<std::is_void_v<decltype(func())>>
{
    if (QThread::currentThread() == qApp->thread()) {
        func();
        return;
    }
    QMetaObject::invokeMethod(
        qApp,
        std::forward<Func>(func),
        Qt::BlockingQueuedConnection
    );
}


QLayout* findContainingLayout(QLayout* layout,QWidget *widget);
QLayout* findContainingLayout(QWidget *widget);

// helper for boost::signals: disconnect at QObject destruction
void disconnectAtEOL(
    QObject *o,
    const boost::signals2::connection& connection
    );




struct FileTypeByExtension
{
    std::string extension;
    std::string description;
    bool isDefault;

    FileTypeByExtension(
        std::string ext = "*",
        std::string desc = "All files",
        bool isDefl = false
        );

    bool operator<(const FileTypeByExtension& o) const;
};



enum GetFileMode { Open, Save };


class getFileName
{
    boost::optional<boost::filesystem::path> selectedPath_;

public:
    getFileName(
     QWidget* parent,
     const QString &caption,
     GetFileMode mode,
     const std::set<FileTypeByExtension>& extensions
        = { {"*", "all files", true} },
     const boost::optional<boost::filesystem::path>& startDir
        = boost::none,
     std::function<void(QGridLayout *)> setupAdditionalControls
        = std::function<void(QGridLayout *)>()
    );


    const boost::filesystem::path& asFilesystemPath() const;
    std::string asString() const;
    QString asQString() const;

    operator bool() const;
    operator QString() const;
    operator boost::filesystem::path() const;
};



/**
 * @brief The IQEphemeralLabel class
 * disappears on mouseclick
 */
class TOOLKIT_GUI_EXPORT IQEphemeralLabel : public QLabel
{
public:
    IQEphemeralLabel(QWidget *parent=nullptr);

    void mousePressEvent(QMouseEvent *event) override;
};



class TOOLKIT_GUI_EXPORT IQSimpleLatexView
    : public QTextEdit
{
    Q_OBJECT

    insight::SimpleLatex content_;
    int cur_content_width_;
    mutable std::pair<int,int> hfwCache_{-1,0}; // width -> heightForWidth
    bool updatingContent_ = false;

    void updateContent();

public:
    IQSimpleLatexView(const insight::SimpleLatex &slt, QWidget *parent = nullptr);
    int heightForWidth(int width) const override;

    QSize sizeHint() const override;

public Q_SLOTS:
    void resizeEvent(QResizeEvent *) override;

};

#endif // QTEXTENSIONS_H
