#ifndef IQFILEDOWNLOADER_H
#define IQFILEDOWNLOADER_H

#include <QtNetwork>
#include <QProgressBar>
#include <QLabel>



class IQFileDownloader : public QObject
{
    Q_OBJECT

    QNetworkAccessManager manager_;
    QNetworkReply * reply_;
    QElapsedTimer downloadTimer_;
    QFile outfile_;

    static bool isHttpRedirect(QNetworkReply *reply);

public:
    IQFileDownloader(const QString &filename, QObject* parent=nullptr);
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



#endif // IQFILEDOWNLOADER_H
