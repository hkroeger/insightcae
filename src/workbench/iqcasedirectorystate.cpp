#include "iqcasedirectorystate.h"

#include "analysisform.h"
#include "ui_analysisform.h"

void IQCaseDirectoryState::updateGUI(bool enabled)
{
  insight::dbg()<<"set IQCaseDirectoryState to enabled="
                <<enabled<<std::endl;
  if (enabled)
    af_->ui->lblWorkingDirectory->setText( *this );
  else
    af_->ui->lblWorkingDirectory->setText("(unset)");

  af_->ui->btnParaview->setEnabled(enabled);
  af_->ui->btnClean->setEnabled(enabled);
  af_->ui->btnShell->setEnabled(enabled);
}



IQCaseDirectoryState::~IQCaseDirectoryState()
{
  updateGUI(false);
}




IQCaseDirectoryState::operator QString() const
{
  return QString::fromStdString( string() );
}
