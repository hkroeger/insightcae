/*
 * bitshift dynamics - Copyright 2012, All rights reserved
 * Author: Alexander Nassian <nassian@bitshift-dynamics.com>
 */

#ifndef EMAIL_H
#define EMAIL_H

#include <QObject>

#include "email_p.h"
#include "mimetypemanager.h"


/**
  * \brief A platform independant wrapper to compose an email and open it int the default mail client.
  *
  * To use it, setup a Email object with the set* and add* methods and call openInDefaultProgram().
  */
class Email : public QObject
{
    Q_OBJECT
public:
    explicit Email(QObject* parent = 0, const QString& mimeTypesPath = QString());
    
public slots:
    void setSenderAddress(const QString& sender)     { p.senderAddress = sender; }
    void setReceiverAddress(const QString& receiver) { p.receiverAddress = receiver; }
    void setSubject(const QString& subject)          { p.subject = subject; }
    void setMessageText(const QString& message)      { p.messageText = message; }

    void addAttachment(const QString& path)          { p.attachments.append(path); }

    void openInDefaultProgram();

signals:
    /**
      * This signal gets fired after openInDefaultProgram() was called and passes a bool
      * that indicates if the mail client was opened successful or not.
      */
    void composerOpened(bool successful);

private:
    EmailPrivate p;
    MimeTypeManager* mimeTypes;
};


#endif // EMAIL_H
