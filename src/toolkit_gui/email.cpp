/*
 * bitshift dynamics - Copyright 2012, All rights reserved
 * Author: Alexander Nassian <nassian@bitshift-dynamics.com>
 */

#include "email.h"

#include <QString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

#include "mimetypemanager.h"


/**
  * \todo Documentation
  */
Email::Email(QObject*parent, const QString& mimeTypesPath) :
    QObject(parent),
    mimeTypes(new MimeTypeManager(mimeTypesPath))
{
}

/**
  * \todo Documentation
  */
void Email::openInDefaultProgram()
{
    QString email = "Content-Type: multipart/alternative; boundary=\"BitshiftDynamicsMailerBoundary\"\r\n";

    // Add sender information


    // Add receiver information
    email.append("To: ");
    email.append(p.receiverAddress);
    email.append("\r\n");

    // Add subject
    email.append("Subject: ");
    email.append(p.subject);
    email.append("\r\n");

    // Add mime version
    email.append("Mime-Version: 1.0 BitshiftDynamics Mailer\r\n\r\n");

    // Add body
    email.append("--BitshiftDynamicsMailerBoundary\r\n");
    email.append("Content-Transfer-Encoding: quoted-printable\r\n");
    email.append("Content-Type: text/plain;\r\n");
    email.append("        charset=iso-8859-1\r\n\r\n");
    email.append(p.messageText);
    email.append("\r\n\r\n");

    // Add attachments
    foreach (QString filePath, p.attachments) {
        QFileInfo fileInfo(filePath);
        QString fileName   = fileInfo.fileName();
        QString fileSuffix = fileInfo.suffix();
        QString mimeType   = mimeTypes->mimeTypeFromExtension(fileSuffix);
        
        if (mimeType.isEmpty())
            mimeType = "text/plain";  // fallback

        QFile attachmentFile(filePath);
        if (attachmentFile.open(QIODevice::ReadOnly) == false) {
            qCritical() << "Failed loading attachment.";

            emit composerOpened(false);
            return;
        }

        QByteArray fileData = attachmentFile.readAll();
        attachmentFile.close();

        email.append("\r\n--BitshiftDynamicsMailerBoundary\r\n");
        email.append("Content-Type: multipart/mixed;\r\n");
        email.append("        boundary=\"BitshiftDynamicsMailerBoundary\"\r\n\r\n");

        email.append("--BitshiftDynamicsMailerBoundary\r\n");
        email.append("Content-Disposition: inline;\r\n");
        email.append("        filename=\"").append(fileName).append("\"\r\n");
        email.append("Content-Type: ").append(mimeType).append(";\r\n");
        email.append("        name=\"").append(fileName).append("\"\r\n");
        email.append("Content-Transfer-Encoding: base64\r\n\r\n");

        email.append(fileData.toBase64());
    }


    // Create temporary file and open it in the user's default email composer
    QString tmpFilePath = QDir::tempPath().append(QString("/ComposedEmail-%1.eml").arg(QDateTime::currentDateTime().toTime_t()));
    QFile tmpFile(tmpFilePath);
    if (tmpFile.open(QIODevice::WriteOnly) == false) {
        qCritical() << "Failed opening temp file for email composing:" << tmpFilePath;

        emit composerOpened(false);
        return;
    }

    tmpFile.write(email.toLatin1());
    tmpFile.close();

    emit composerOpened(QDesktopServices::openUrl(QUrl(tmpFilePath)));
}
