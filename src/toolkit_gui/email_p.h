/*
 * bitshift dynamics - Copyright 2012, All rights reserved
 * Author: Alexander Nassian <nassian@bitshift-dynamics.com>
 */

#ifndef EMAILPRIVATE_H
#define EMAILPRIVATE_H

#include <QString>
#include <QList>


/**
  * PIMPL pattern object for QxtEmail to avoid dirty namings.
  */
struct EmailPrivate
{
    QString senderAddress;
    QString receiverAddress;
    QString subject;
    QString messageText;

    QList<QString> attachments;
};


#endif // EMAILPRIVATE_H
