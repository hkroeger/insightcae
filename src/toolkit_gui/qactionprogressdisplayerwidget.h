#ifndef INSIGHT_QACTIONPROGRESSDISPLAYERWIDGET_H
#define INSIGHT_QACTIONPROGRESSDISPLAYERWIDGET_H

#include "toolkit_gui_export.h"


#include <QWidget>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMap>

#include "base/progressdisplayer.h"
#include "iqbackgroundtask.h"

namespace insight {


class TOOLKIT_GUI_EXPORT IQProgressWidget
    : public QWidget
{
    Q_OBJECT

public:
    typedef std::pair<ProgressDisplayer*,std::string> Action;

protected:
    Action          trackedAction_;
    QFrame*         card = nullptr;
    QLabel*         titleLabel_    = nullptr;
    QLabel*         messageLabel_  = nullptr;
    QPushButton*    cancelButton_  = nullptr;
    QProgressBar*   progressBar_   = nullptr;

    void setupUi(const QString& title);
    void setCancelButton(bool enabled);
    void recheckCancelButton();

public:
    IQProgressWidget(
        const QString& progressTitle,
        const Action& trackedAction,
        QWidget* parent = nullptr );
    ~IQProgressWidget();

public Q_SLOTS:
    void onStatusMessage(const QString& message);
    void onProgressChanged(int percent);
    void onCancelClicked();

Q_SIGNALS:
    /// Emitted just before this widget is about to be destroyed.
    void widgetClosing(IQProgressWidget* self);
};





class TOOLKIT_GUI_EXPORT IQActionProgressDisplayManager
: public QObject,
  public insight::ProgressDisplayer
{
    Q_OBJECT

    QWidget*                  hostWidget_ = nullptr;
    QWidget*                  statusBar_  = nullptr;
    std::map<std::string, IQProgressWidget*>  overlays_;

    static constexpr int k_margin  = 2;  ///< Distance from the window edge.
    static constexpr int k_spacing = 1;  ///< Vertical gap between cards.
    static constexpr int k_indent  = 16; ///< Horizontal indent per depth level for child actions.


protected:
    IQProgressWidget* getItem(const std::string& path, bool createIfNonExistent = true);

    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void relayout();
    void onWidgetClosing(IQProgressWidget* widget);

public:
    IQActionProgressDisplayManager(QWidget* parent, QWidget* statusBar = nullptr);

    void update ( const ProgressState& pi ) override;
    void logMessage(const std::string& line) override;
    void setActionProgressValue(const std::string& path, double value) override;
    void setMessageText(const std::string& path, const std::string& message) override;
    void finishActionProgress(const std::string& path) override;
    void reset() override;
};



} // namespace insight

#endif // INSIGHT_QPROGRESSDISPLAYERWIDGET_H
