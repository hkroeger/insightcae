#ifndef IQSERVERLAUNCHDIALOG_H
#define IQSERVERLAUNCHDIALOG_H

#include "toolkit_gui_export.h"

#include <QDialog>

#include <functional>
#include <thread>
#include <exception>

#include "base/remoteserver.h"

class QPlainTextEdit;
class QLabel;
class QDialogButtonBox;


/**
 * @brief The IQServerLaunchDialog class
 * performs an action on a remote server (launch, shutdown)
 * in background and displays its console output meanwhile.
 * The dialog cannot be closed, while the action is in progress.
 */
class TOOLKIT_GUI_EXPORT IQServerLaunchDialog
    : public QDialog
{
    Q_OBJECT

public:
    typedef std::function<void(insight::RemoteServer::Config::OutputLineCallback)> ServerAction;

private:
    ServerAction action_;
    QString failureText_;
    QPlainTextEdit* log_;
    QLabel* status_;
    QDialogButtonBox* buttons_;

    std::thread launchThread_;
    bool running_ = false;
    std::exception_ptr error_;

    void appendLine(const QString& line);
    void launchFinished(std::exception_ptr error);

protected:
    void closeEvent(QCloseEvent* e) override;

public:
    IQServerLaunchDialog(
        const QString& title,
        const QString& waitText,
        const QString& failureText,
        ServerAction action,
        QWidget* parent = nullptr );
    ~IQServerLaunchDialog();

    int exec() override;
    void reject() override;

    /**
     * @brief launchServer
     * launch the server and show the output.
     * On success, the dialog closes automatically.
     * On failure, it stays open, until the user closes it.
     * @return
     * true, if the server was launched successfully
     */
    static bool launchServer(
        const insight::RemoteServer::Config& serverConfig,
        QWidget* parent = nullptr );

    /**
     * @brief shutdownServer
     * execute the destruction command of the server and show the output.
     * Same behaviour as launchServer.
     */
    static bool shutdownServer(
        const insight::RemoteServer::Config& serverConfig,
        QWidget* parent = nullptr );
};


#endif // IQSERVERLAUNCHDIALOG_H
