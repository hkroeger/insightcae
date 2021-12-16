#ifndef IQSETUPWSLDISTRIBUTIONWIZARD_H
#define IQSETUPWSLDISTRIBUTIONWIZARD_H

#include <QDialog>

#include <QtNetwork>
#include <QProgressBar>
#include <QLabel>

namespace Ui {
class IQSetupWSLDistributionWizard;
}


class FileDownloader : public QObject
{
    Q_OBJECT

    QNetworkAccessManager manager_;
    QNetworkReply * reply_;
    QElapsedTimer downloadTimer_;
    QFile outfile_;

    static bool isHttpRedirect(QNetworkReply *reply);

public:
    FileDownloader(const QString &filename, QObject* parent=nullptr);
    void start(const QUrl &url);

    void connectProgressBar(QProgressBar* pb);
    void connectLabel(QLabel* label);

Q_SIGNALS:
    void finished(const QString& filename);
    void setProgressMax(qint64 max);
    void setProgressCurrent(qint64 cp);
    void setProgressMessage(const QString& msg);
    void failed(const QString& errorMsg);

private Q_SLOTS:
    void downloadFinished(QNetworkReply *reply);
    void sslErrors(const QList<QSslError> &errors);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};


class WaitAnimation : public QObject
{
    Q_OBJECT

    QLabel* label_;
    QString baseMsg_;
    int nDots_;
    QTimer timer_;

public:
    WaitAnimation(const QString& baseMsg, QLabel* out);
    ~WaitAnimation();
};




class IQSetupWSLDistributionWizard : public QDialog
{
    Q_OBJECT

    QString effectiveRepoURL() const;

    QPointer<WaitAnimation> wanim_;

    const QString wslexe_ = "c:\\windows\\sysnative\\wsl.exe";

    QProcess* launchSubprocess(
            const QString& cmd,
            const QStringList& args,
            const QString& explainText,
            std::function<void()> nextStep
            );

private Q_SLOTS:
    void start();
    void downloadWSLImage();
    void createWSLDistribution(const QString& imagefile);
    void configureWSLDistribution();
    void restartWSLDistribution();
    void completed();

    void failed(const QString& errorMsg);

    QTemporaryFile* createSetupScript();

public:
    explicit IQSetupWSLDistributionWizard(QWidget *parent = nullptr);
    ~IQSetupWSLDistributionWizard();

    void accept() override;

    QString distributionLabel() const;
    QString baseDirectory() const;

private:
    Ui::IQSetupWSLDistributionWizard *ui;
};

#endif // IQSETUPWSLDISTRIBUTIONWIZARD_H
