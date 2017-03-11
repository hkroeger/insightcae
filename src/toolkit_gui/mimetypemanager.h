/*
 * bitshift dynamics - Copyright 2012, All rights reserved
 * Author: Alexander Nassian <nassian@bitshift-dynamics.com>
 */

#ifndef MIMETYPEMANAGER_H
#define MIMETYPEMANAGER_H

#include <QMap>
#include <QString>


/**
  * \brief This class provides a simple mime.types file reader.
  *
  * The mime.types file is used from the apache project where it maps filename suffixes to
  * their mimetype.
  *
  * To send an email with attachments we need to define a mimetype for every attachment. It
  * would be safer to determine the mimetype by the file's data structure like the magic number
  * but this would be too complex and this simple solution works for 99% of the cases covered
  * by this module.
  */
class MimeTypeManager
{
public:
    explicit MimeTypeManager(const QString&);

    QMap<QString, QString> allMimeTypes() { return mimeTypes; }

    QString mimeTypeFromExtension(const QString& extension) { return mimeTypes[extension]; }

private:
    QMap<QString, QString> mimeTypes;
};


#endif // MIMETYPEMANAGER_H
