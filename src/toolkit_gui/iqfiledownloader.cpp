#include "iqfiledownloader.h"

#include "base/exception.h"



IQFileDownloader::IQFileDownloader(const QString &filename, QObject* parent)
    : QObject(parent),
      outfile_(filename)
{
    connect(&manager_, &QNetworkAccessManager::finished,
            this, &IQFileDownloader::downloadFinished);
}




void IQFileDownloader::start(const QUrl &url)
{
    if (!outfile_.open(QIODevice::WriteOnly))
    {
        Q_EMIT failed(
                    QString("Could not open %1 for writing: %2")
                    .arg(
                        outfile_.fileName(),
                        outfile_.errorString()
                        )
                    );
        return;
    }


    QNetworkRequest request(url);
    reply_ = manager_.get(request);

    connect(reply_, &QNetworkReply::readyRead, reply_,
            [&]()
            {
                outfile_.write( reply_->readAll() );
            }
    );

#if QT_CONFIG(ssl)
    connect(reply_, &QNetworkReply::sslErrors,
            this, &IQFileDownloader::sslErrors);
#endif

    downloadTimer_.start();
    connect(reply_, &QNetworkReply::downloadProgress,
            this, &IQFileDownloader::downloadProgress);
}




void IQFileDownloader::connectProgressBar(QProgressBar* pb)
{
    connect(this, &IQFileDownloader::setProgressMax,
            pb, &QProgressBar::setMaximum);
    connect(this, &IQFileDownloader::setProgressCurrent,
            pb, &QProgressBar::setValue);
}




void IQFileDownloader::connectLabel(QLabel *label)
{
    connect(this, &IQFileDownloader::setProgressMessage,
            label, &QLabel::setText );
}




void IQFileDownloader::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    insight::dbg()<<"downloadProgress recv:"<<bytesReceived<<", total:"<<bytesTotal<<std::endl;
    Q_EMIT setProgressMax(bytesTotal);
    Q_EMIT setProgressCurrent(bytesReceived);

    // calculate the download speed
    double speed = bytesReceived * 1000.0 / downloadTimer_.elapsed();
    QString unit;
    if (speed < 1024) {
        unit = "bytes/sec";
    } else if (speed < 1024*1024) {
        speed /= 1024;
        unit = "kB/s";
    } else {
        speed /= 1024*1024;
        unit = "MB/s";
    }

    Q_EMIT setProgressMessage(QString::fromLatin1("%1 %2")
                           .arg(speed, 3, 'f', 1).arg(unit));
}




bool IQFileDownloader::isHttpRedirect(QNetworkReply *reply)
{
    insight::dbg()<<"isHttpRedirect"<<std::endl;
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303
           || statusCode == 305 || statusCode == 307 || statusCode == 308;
}




void IQFileDownloader::sslErrors(const QList<QSslError> &sslErrors)
{
    insight::dbg()<<"sslErrors"<<std::endl;
#if QT_CONFIG(ssl)
    for (const QSslError &error : sslErrors)
        fprintf(stderr, "SSL error: %s\n", qPrintable(error.errorString()));
#else
    Q_UNUSED(sslErrors);
#endif
}




void IQFileDownloader::downloadFinished(QNetworkReply *reply)
{
    insight::dbg()<<"downloadFinished"<<std::endl;
    QUrl url = reply->url();
    if (reply->error())
    {
        Q_EMIT failed(
                    QString("Download of %1 failed:\n%2")
                    .arg(url.toEncoded(),
                         reply->errorString())
                    );
    }
    else
    {
        if (isHttpRedirect(reply))
        {
            Q_EMIT failed("Request was redirected");
        }
        else
        {
            outfile_.write(reply->readAll());
            outfile_.close();

            Q_EMIT setProgressMessage(
                            QString("Download of %1 succeeded (saved to %2)")
                            .arg(
                                url.toEncoded(),
                                outfile_.fileName())
                            );

            Q_EMIT finished(outfile_.fileName());
            deleteLater();
        }
    }

    reply->deleteLater();
}

