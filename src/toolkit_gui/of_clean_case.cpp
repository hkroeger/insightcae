
#include "of_clean_case.h"
#include "ui_of_clean_case.h"

#include <QListWidgetItem>

namespace bf=boost::filesystem;

OFCleanCaseForm::OFCleanCaseForm(const insight::OpenFOAMCase& ofc, const boost::filesystem::path& location, QWidget* parent)
  : QWidget(parent),
    ofc_(ofc),
    location_(location)
{
  ui=new Ui::OFCleanCaseForm();
  ui->setupUi(this);

  t_=new QTimer(this);
  t_->setSingleShot(true);
  connect(t_, &QTimer::timeout,
          [=]() { emit statusMessage(""); });

  ui->dirname->setText(QString::fromStdString(location_.string()));

  connect(ui->cb_clean_times, &QCheckBox::toggled,
          [=](bool checked) {
           ui->cb_keep_first->setDisabled(!checked);
         });

  connect(ui->cb_clean_all, &QCheckBox::toggled,
          [=](bool checked) {
//           if (checked)
//            {
//              if (ui->cb_keep_first->isChecked())
//              {
//                 ui->cb_keep_first->setChecked(false);
//              }
//            }
           ui->cb_clean_times->setDisabled(checked);
           if (checked)
           {
             ui->cb_keep_first->setDisabled(true);
           }
           else
           {
             ui->cb_keep_first->setDisabled(!ui->cb_clean_times->isChecked());
           }
           ui->cb_clean_post->setDisabled(checked);
           ui->cb_clean_proc->setDisabled(checked);
         });

  connect(ui->cb_clean_times, &QCheckBox::toggled,
          this, &OFCleanCaseForm::updateCandidateList);
  connect(ui->cb_keep_first, &QCheckBox::toggled,
          this, &OFCleanCaseForm::updateCandidateList);
  connect(ui->cb_clean_post, &QCheckBox::toggled,
          this, &OFCleanCaseForm::updateCandidateList);
  connect(ui->cb_clean_proc, &QCheckBox::toggled,
          this, &OFCleanCaseForm::updateCandidateList);
  connect(ui->cb_clean_all, &QCheckBox::toggled,
          this, &OFCleanCaseForm::updateCandidateList);

  connect(ui->btn_exec, &QPushButton::clicked,
          this, &OFCleanCaseForm::executeDeletion);

  updateCandidateList();

}


insight::OpenFOAMCaseDirs::TimeDirOpt OFCleanCaseForm::timeStepSelection()
{
  insight::OpenFOAMCaseDirs::TimeDirOpt cto = insight::OpenFOAMCaseDirs::TimeDirOpt::All;
  if (ui->cb_keep_first->isChecked() && ui->cb_keep_first->isEnabled())
  {
    cto=insight::OpenFOAMCaseDirs::TimeDirOpt::ExceptFirst;
  }
  return cto;
}

void OFCleanCaseForm::updateCandidateList()
{
  cf_.reset(new insight::OpenFOAMCaseDirs(ofc_, location_));

  bool cleanproc=ui->cb_clean_proc->isChecked() || ui->cb_clean_all->isChecked();
  bool cleantimes=ui->cb_clean_times->isChecked() || ui->cb_clean_all->isChecked();
  bool cleanpost=ui->cb_clean_post->isChecked() || ui->cb_clean_all->isChecked();
  bool cleanall=ui->cb_clean_all->isChecked();

  std::set<boost::filesystem::path> cands = cf_->caseFilesAndDirs
  (
      timeStepSelection(),
      cleanproc, cleantimes, cleanpost, cleanall
  );

  ui->list_cand->clear();
  for (auto& c: cands)
  {
    boost::filesystem::path rp = make_relative(location_, c);
    ui->list_cand->addItem(
          new QListWidgetItem(QString::fromStdString(rp.string()), ui->list_cand)
          );
  }
  if (cands.size()==0)
  {
    ui->list_cand->addItem(
          new QListWidgetItem("(nothing selected)", ui->list_cand)
          );
  }
}

void OFCleanCaseForm::executeDeletion()
{
  if (!cf_)
    cf_.reset(new insight::OpenFOAMCaseDirs(ofc_, location_));

  t_->stop();

  if (ui->cb_pack->isChecked())
  {
      bf::path archive_file;

//      if (vm.count("pack-file"))
//      {
//        archive_file = vm["pack-file"].as<std::string>();
//      }
//      else
//      {

        boost::posix_time::ptime t( boost::posix_time::microsec_clock::local_time() );
        boost::posix_time::time_facet *facet = new boost::posix_time::time_facet();
        facet->format( ( bf::current_path().filename().string()+"_%Y-%m-%d-%H.%M" ).c_str() );

        std::ostringstream stream;
        stream.imbue(std::locale(std::locale::classic(), facet));
        stream << t;

        archive_file = location_ / (stream.str()+".tar.gz");
//      }

      emit statusMessage( "Packing case to file "+QString::fromStdString(archive_file.string()) );

      cf_->packCase(archive_file, insight::OpenFOAMCaseDirs::TimeDirOpt::OnlyFirstAndLast);
  }

  bool cleanproc=ui->cb_clean_proc->isChecked() || ui->cb_clean_all->isChecked();
  bool cleantimes=ui->cb_clean_times->isChecked() || ui->cb_clean_all->isChecked();
  bool cleanpost=ui->cb_clean_post->isChecked() || ui->cb_clean_all->isChecked();
  bool cleanall=ui->cb_clean_all->isChecked();

  emit statusMessage("Deleting...");
  cf_->cleanCase
      (
        timeStepSelection(),
        cleanproc,
        cleantimes,
        cleanpost,
        cleanall
      );

  emit statusMessage("Cleanup finished.");

  updateCandidateList();

  t_->start(2000);
}


OFCleanCaseDialog::OFCleanCaseDialog(const insight::OpenFOAMCase& ofc, const boost::filesystem::path& location, QWidget* parent)
  : QDialog(parent)
{
  QVBoxLayout *l=new QVBoxLayout;
  setLayout(l);
  auto* ofcc = new OFCleanCaseForm(ofc, location, this);
  l->addWidget(ofcc);
  QLabel *status=new QLabel(this);
  l->addWidget(status);
  connect(ofcc, &OFCleanCaseForm::statusMessage,
          status, &QLabel::setText);
}
