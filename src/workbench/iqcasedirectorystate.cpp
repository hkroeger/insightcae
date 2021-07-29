#include "iqcasedirectorystate.h"

#include "analysisform.h"
#include "ui_analysisform.h"

void IQCaseDirectoryState::setAFEnabledState(bool enabled)
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




IQCaseDirectoryState::IQCaseDirectoryState(AnalysisForm *af, const boost::filesystem::path& path, bool keep)
  : insight::CaseDirectory(path, keep),
    af_(af)
{
  setAFEnabledState(true);
}




IQCaseDirectoryState::IQCaseDirectoryState(AnalysisForm *af, bool keep, const boost::filesystem::path& prefix)
  : insight::CaseDirectory(keep, prefix),
    af_(af)
{
  setAFEnabledState(true);
}




IQCaseDirectoryState::~IQCaseDirectoryState()
{
  setAFEnabledState(false);
}




IQCaseDirectoryState::operator QString() const
{
  return QString::fromStdString( string() );
}
