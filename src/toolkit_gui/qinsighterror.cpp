#include "qinsighterror.h"

#include <QMessageBox>

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
