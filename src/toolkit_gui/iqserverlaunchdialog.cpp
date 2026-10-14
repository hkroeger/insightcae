#include "iqserverlaunchdialog.h"

#include "base/exception.h"
#include "base/translations.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>



IQServerLaunchDialog::IQServerLaunchDialog(
    const QString& title,
    const QString& waitText,
    const QString& failureText,
    ServerAction action,
    QWidget* parent )
  : QDialog(parent),
    action_(action),
    failureText_(failureText)
{
    setWindowTitle(title);

    auto *l = new QVBoxLayout(this);

    status_ = new QLabel(waitText);
    l->addWidget(status_);

    log_ = new QPlainTextEdit;
    log_->setReadOnly(true);
    log_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    log_->setMaximumBlockCount(5000);
    l->addWidget(log_);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons_->button(QDialogButtonBox::Close)->setEnabled(false);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    l->addWidget(buttons_);

    resize(700, 350);
}




IQServerLaunchDialog::~IQServerLaunchDialog()
{
    if (launchThread_.joinable())
        launchThread_.join();
}




void IQServerLaunchDialog::appendLine(const QString& line)
{
    auto *sb = log_->verticalScrollBar();
    bool atEnd = (sb->value() == sb->maximum());
    log_->appendPlainText(line);
    if (atEnd) sb->setValue(sb->maximum());
}




void IQServerLaunchDialog::launchFinished(std::exception_ptr error)
{
    if (launchThread_.joinable())
        launchThread_.join();

    running_ = false;
    error_ = error;
    buttons_->button(QDialogButtonBox::Close)->setEnabled(true);

    if (!error_)
    {
        accept();
    }
    else
    {
        QString msg;
        try { std::rethrow_exception(error_); }
        catch (const std::exception& e) { msg = QString::fromStdString(e.what()); }
        catch (...) { msg = _("unknown error"); }

        status_->setText(
            QString("<b>%1</b> %2")
                .arg(failureText_.toHtmlEscaped(), msg.toHtmlEscaped()) );
        appendLine(QString("*** ")+msg);
    }
}




int IQServerLaunchDialog::exec()
{
    running_ = true;

    // the output callback and the result are posted into the GUI thread.
    // The thread is joined before this dialog is destroyed,
    // so "this" stays valid.
    launchThread_ = std::thread(
        [this]()
        {
            std::exception_ptr error;
            try
            {
                action_(
                    [this](const std::string& line)
                    {
                        auto l = QString::fromStdString(line);
                        QMetaObject::invokeMethod(
                            this, [this,l]() { appendLine(l); },
                            Qt::QueuedConnection );
                    } );
            }
            catch (...)
            {
                error = std::current_exception();
            }
            QMetaObject::invokeMethod(
                this, [this,error]() { launchFinished(error); },
                Qt::QueuedConnection );
        } );

    return QDialog::exec();
}




void IQServerLaunchDialog::reject()
{
    if (running_) return; // the launch cannot be interrupted
    QDialog::reject();
}




void IQServerLaunchDialog::closeEvent(QCloseEvent* e)
{
    if (running_)
        e->ignore();
    else
        QDialog::closeEvent(e);
}




bool IQServerLaunchDialog::launchServer(
    const insight::RemoteServer::Config& serverConfig,
    QWidget* parent )
{
    auto label = QString::fromStdString(serverConfig);
    IQServerLaunchDialog dlg(
        QString(_("Launching server %1")).arg(label),
        _("Launching the server (creating its instance, if required), please wait..."),
        _("The server could not be launched:"),
        [&serverConfig](insight::RemoteServer::Config::OutputLineCallback ol)
        { serverConfig.launch(ol); },
        parent );
    return dlg.exec() == QDialog::Accepted;
}




bool IQServerLaunchDialog::shutdownServer(
    const insight::RemoteServer::Config& serverConfig,
    QWidget* parent )
{
    auto label = QString::fromStdString(serverConfig);
    IQServerLaunchDialog dlg(
        QString(_("Shutting down server %1")).arg(label),
        _("Shutting down the server, please wait..."),
        _("The server could not be shut down:"),
        [&serverConfig](insight::RemoteServer::Config::OutputLineCallback ol)
        { serverConfig.shutdown(ol); },
        parent );
    return dlg.exec() == QDialog::Accepted;
}




bool IQServerLaunchDialog::disposeServer(
    const insight::RemoteServer::Config& serverConfig,
    QWidget* parent )
{
    auto label = QString::fromStdString(serverConfig);
    IQServerLaunchDialog dlg(
        QString(_("Disposing server %1")).arg(label),
        _("Shutting down and deleting the server instance, please wait..."),
        _("The server could not be disposed:"),
        [&serverConfig](insight::RemoteServer::Config::OutputLineCallback ol)
        { serverConfig.dispose(ol); },
        parent );
    return dlg.exec() == QDialog::Accepted;
}
