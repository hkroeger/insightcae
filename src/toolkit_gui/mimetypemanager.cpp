/*
 * bitshift dynamics - Copyright 2012, All rights reserved
 * Author: Alexander Nassian <nassian@bitshift-dynamics.com>
 */

#include "mimetypemanager.h"

#include <QFile>
#include <QDebug>
#include <QStringList>


/**
  * This overloaded constructor loads a mime.types file with the given path and stores it's
  * contents in an internal QMap.
  *
  * @param filePath The path to the mime.types file.
  */
MimeTypeManager::MimeTypeManager(const QString& filePath)
{
    QFile mimeFile(filePath);
    if (mimeFile.open(QIODevice::ReadOnly) == false) {
        qCritical() << "Failed opening mime type mapping.";
        return;
    }

    QTextStream stream(&mimeFile);

    while (mimeFile.atEnd() == false) {
        QString currentLine = stream.readLine();

        // Ignore commented lines
        if (currentLine.startsWith("#") == true)
            continue;

        // Split the current entry and filter invalid entries
        QStringList components = currentLine.split("\t", QString::SkipEmptyParts);
        if (components.count() != 2) {
            qWarning() << "Invalid entry.";
            continue;
        }

        // Split multiple filename suffixes for every mimetype
        QStringList endings = components.at(1).split(" ", QString::SkipEmptyParts);
        QString mimeType = components.at(0);

        foreach (QString ending, endings) {
            mimeTypes[ending] = mimeType;
        }
    }

    mimeFile.close();
}
