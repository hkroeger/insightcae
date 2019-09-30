#include "qinsighterror.h"

#include <QMessageBox>
#include "Standard_Failure.hxx"

void displayException(const std::exception& e)
{
  // put it to console as well...
  insight::printException(e);

  if (const auto* ie = dynamic_cast<const insight::Exception*>(&e))
  {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText
        (
          QString("An error has occurred:\n")+ie->as_string().c_str()
        );

    if (ie->strace()!="")
    {
      msgBox.setInformativeText("See stack trace for more details on error origin.");
      msgBox.setDetailedText(ie->strace().c_str());
    }

    msgBox.exec();
  }

  else if (const auto* ie = dynamic_cast<const Standard_Failure*>(&e))
  {
    // this is not handled in "printException"
    std::cerr << std::endl
              << "An error has occurred during CAD geometry processing:" << std::endl
              << ie->GetMessageString() << std::endl
                 ;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText
        (
          QString("An error has occurred during CAD geometry processing:\n")+ie->GetMessageString()
        );

    msgBox.exec();
  }

  else
  {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText
        (
          QString("An error has occurred:\n")+e.what()
        );

    msgBox.exec();
  }

}
