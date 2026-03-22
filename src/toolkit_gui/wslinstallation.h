#ifndef INSIGHT_WSLINSTALLATION_H
#define INSIGHT_WSLINSTALLATION_H

#include "base/wsllinuxserver.h"
#include "base/progressdisplayer.h"

#include <QDialog>
#include <QWidget>
#include <QThread>
#include <QPlainTextEdit>
#include <QPushButton>


namespace insight {

void checkWSLVersions(ActionProgress& prog, QWidget *parent=nullptr);
void checkExternalPrograms(ActionProgress& prog, QWidget *parent=nullptr);

void launchUpdateWSLVersion(
        std::shared_ptr<insight::WSLLinuxServer> wsl,
        QWidget *parent = nullptr );

/**
 * @brief launchUpdateWSLVersionAndWait
 * Like launchUpdateWSLVersion but blocks the caller until the update thread
 * finishes. Must be called from the GUI thread (e.g. inside runInGUIThread).
 * A nested QEventLoop keeps the GUI responsive while waiting.
 */
void launchUpdateWSLVersionAndWait(
        std::shared_ptr<insight::WSLLinuxServer> wsl,
        QWidget *parent = nullptr );


class UpdateWSLVersionThread : public QThread
{
    Q_OBJECT

    QWidget *parent_;
    std::shared_ptr<insight::WSLLinuxServer> wsl_;
    bool success_;

public:
    UpdateWSLVersionThread(
            std::shared_ptr<insight::WSLLinuxServer> wsl,
            QWidget *parent = nullptr );

    void run() override;

    bool success() const;

Q_SIGNALS:
    void logLine(const QString& line);
};


class UpdateWSLProgressDialog
        : public QDialog
{
    Q_OBJECT

    QPlainTextEdit *pte_;
    QPushButton *btn_;

public:
    UpdateWSLProgressDialog(QWidget *parent = nullptr);

    void changeCloseBtnText(const QString& label);

public Q_SLOTS:
    void appendLogLine(const QString& line);
};


} // namespace insight

#endif // INSIGHT_WSLINSTALLATION_H
