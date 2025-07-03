#include "qinsighterror.h"

#include <QMessageBox>
#include "Standard_Failure.hxx"
#include "base/exception.h"
#include "base/translations.h"
#include "cadexception.h"

#include "iqcadexceptiondisplaydialog.h"




void displayCurrentException(QWidget *parentWidget)
{
    auto desc = insight::describeCurrentException();

    if (auto cadex =
            std::dynamic_pointer_cast<insight::CADException>(
                desc ))
    {
        IQCADExceptionDisplayDialog msg;
        msg.displayException(*cadex->description());
        msg.exec();
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);

        QString text=
            QString(_("An error has occurred"))+":\n"
            + QString::fromStdString(*desc)
            ;

        if (!desc->context_.empty())
        {
            text += "\n"
              + QString::fromStdString(desc->context_);
        }

        msgBox.setText(text);

        QString details;

        if (desc->errorDetails_!="")
        {
            details +=
                QString::fromStdString(desc->errorDetails_);
        }

        if (desc->strace_!="")
        {
            if (!details.isEmpty()) details += "\n\n";

            details +=
                QString(_("Stack trace"))+"\n"
                + QString::fromStdString(desc->strace_);
        }

        if (!details.isEmpty())
        {
            msgBox.setInformativeText(_("More details on error origin."));
            msgBox.setDetailedText(details);
        }

        msgBox.exec();
    }
}
