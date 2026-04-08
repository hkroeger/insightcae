#include "wslinstallation.h"

#include "base/remoteserver.h"
#include "base/tools.h"
#include "base/remoteserverlist.h"
#include "base/externalprograms.h"
#include "iqsetupwsldistributionwizard.h"
#include "iqremoteservereditdialog.h"
#include "iqconfigureexternalprogramsdialog.h"
#include "qtextensions.h"

#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QEventLoop>
#include <qnamespace.h>

namespace insight {





void checkWSLVersions(ActionProgress& prog, QWidget* parent)
{
  bool anythingChecked=false, anythingOutdated=false;

  // ===============================================================================================
  // ====== go through all defined remote servers and check them, if they are WSL remotes

  prog.setNSteps(insight::remoteServers().size());

  for (auto& rs: insight::remoteServers())
  {
    if ( auto wslcfg = std::dynamic_pointer_cast<insight::WSLLinuxServer::Config>(rs) )
    {
      // check installed version inside WSL distribution
      if (auto wsl = std::dynamic_pointer_cast<insight::WSLLinuxServer>(wslcfg->instance()))
      {
        prog.stepUp(str(boost::format("Checking WSL distribution %s...")
                          %wsl->myDistributionLabel()));

        try
        {
          auto wslVersion = wsl->checkInstalledVersion();
          anythingChecked=true;
          if ( !(wslVersion == insight::ToolkitVersion::current()) )
          {
            anythingOutdated=true;
            auto answer = runInGUIThread([&]() {
              return QMessageBox::warning(
                    parent,
                    "Inconsistent configuration",
                    "The installed InsightCAE package in the WSL distribution \""+QString::fromStdString(wslcfg->distributionLabel_)+"\"\n"
                    "(referenced in remote server configuration \""+QString::fromStdString(*wslcfg)+"\")\n"
                    "is not of the same version as this GUI frontend!\n\n"
                    " GUI version: "+QString::fromStdString(insight::ToolkitVersion::current().toString())+"\n"
                    " WSL version: "+QString::fromStdString(wslVersion.toString())+")\n\n"
                    "Please check the reason. If you recently updated the GUI package, please update the backend before executing analyses.\n"
                    "\nExecute the backend update now?",
                    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
                    );
            });
            if (answer==QMessageBox::Yes)
            {
              runInGUIThread([&]() {
                launchUpdateWSLVersionAndWait( wsl, parent );
              });
            }
          }
        }
        catch (const insight::Exception& ex)
        {
          runInGUIThread([&]() {
            QMessageBox::warning(
                  parent,
                  "Inconsistent configuration",
                  "Installed backend version of WSL distribution:\n"
                  " "+QString::fromStdString(wslcfg->distributionLabel_)+"\n"
                  " (referenced in remote server \""+QString::fromStdString(*wslcfg)+"\")\n"
                  "could not be checked.\n"+
                  QString::fromStdString(ex.message())
                  );
          });
        }
      }
    }
  }




  // ===============================================================================================
  // ====== there is no WSL backend yet

  if (!anythingChecked)
  {
      std::exception_ptr eptr;
      runInGUIThread([&]() {
          QMessageBox qb(
                      QMessageBox::Question,
                      "No WSL backend",
                      "There is no WSL backend defined yet!\n"
                      "At least one WSL backend is required to perform simulations.\n"
                      "Do you want to create one now?",
                      QMessageBox::NoButton,
                      parent
                      );
          auto *createBtn = qb.addButton("Create", QMessageBox::ActionRole);
          auto *selectBtn = qb.addButton("Select existing", QMessageBox::ActionRole);
          auto *cancelBtn = qb.addButton("Cancel", QMessageBox::ActionRole);
          qb.exec();
          if (qb.clickedButton()==createBtn)
          {
              IQSetupWSLDistributionWizard wizdlg(parent);
              if (wizdlg.exec() == QDialog::Accepted)
              {
                  try
                  {
                      auto wslcfg = std::make_shared<insight::WSLLinuxServer::Config>(
                            wizdlg.baseDirectory().toStdString(),
                            std::thread::hardware_concurrency(),
                            wizdlg.distributionLabel().toStdString()
                            );
                      auto lbl = insight::findUnusedLabel(
                                  insight::remoteServers().begin(),
                                  insight::remoteServers().end(),
                                  wizdlg.distributionLabel().toStdString(),
                                  [](insight::RemoteServerList::iterator it)
                                   -> std::string
                                  {
                                    return static_cast<std::string>(*(*it));
                                  }
                                  );
                      static_cast<std::string&>(*wslcfg) = lbl;

                      insight::remoteServers().insert(wslcfg);

                      auto serverlist = insight::remoteServers().firstWritableLocation();
                      if (serverlist.empty())
                      {
                          throw insight::Exception("There is no writable location for the remoteservers.list file!\n"
                                                   "Please contact your system administrator or the support!");
                      }
                      insight::remoteServers().writeConfiguration(serverlist);
                  }
                  catch (...)
                  {
                      eptr = std::current_exception();
                  }
              }
          }
          else if (qb.clickedButton()==selectBtn)
          {
              IQRemoteServerEditDialog dlg(parent);
              dlg.exec();
          }
      });
      if (eptr) std::rethrow_exception(eptr);
  }
}





void launchUpdateWSLVersion(std::shared_ptr<WSLLinuxServer> wsl, QWidget *parent)
{
    auto *t = new UpdateWSLVersionThread( wsl, parent );
    auto *pw = new UpdateWSLProgressDialog(parent);

    QObject::connect(
                t, &UpdateWSLVersionThread::logLine,
                pw, &UpdateWSLProgressDialog::appendLogLine);
    pw->show();

    QObject::connect(
                t, &UpdateWSLVersionThread::finished,
                [t,pw]()
                {
                    if (t->success())
                    {
                        QMetaObject::invokeMethod(
                            qApp,
                            std::bind(&UpdateWSLProgressDialog::close, pw)
                        );
                    }
                    QMetaObject::invokeMethod(
                        qApp,
                        std::bind(&UpdateWSLProgressDialog::changeCloseBtnText, pw, "Close")
                    );
                }
    );
    t->start();
}



void launchUpdateWSLVersionAndWait(std::shared_ptr<WSLLinuxServer> wsl, QWidget* parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    auto *t  = new UpdateWSLVersionThread(wsl, parent);
    auto *pw = new UpdateWSLProgressDialog(parent);

    QObject::connect(
                t, &UpdateWSLVersionThread::logLine,
                pw, &UpdateWSLProgressDialog::appendLogLine);
    pw->show();

    QEventLoop loop;
    QObject::connect(t, &QThread::finished, &loop, &QEventLoop::quit);
    QObject::connect(
                t, &UpdateWSLVersionThread::finished,
                [t, pw]()
                {
                    if (t->success())
                        pw->close();
                    else
                        pw->changeCloseBtnText("Close");
                });
    t->start();
    loop.exec();
}



UpdateWSLVersionThread::UpdateWSLVersionThread(
        std::shared_ptr<WSLLinuxServer> wsl, QWidget *parent)
    : wsl_(wsl),
      parent_(parent)
{
    connect(this, &UpdateWSLVersionThread::finished,
            this, &QObject::deleteLater);
}



void UpdateWSLVersionThread::run()
{
    try
    {
        //          parent->statusBar()->showMessage("Update of WSL backend instance is running.");
        wsl_->updateInstallation(
                    [this](const std::string& line)
                    {
                        Q_EMIT logLine(QString::fromStdString(line));
                    }
                    );
        success_=true;
    }
    catch (const std::exception& ex)
    {
        success_=false;
        QMetaObject::invokeMethod(
            qApp,
            [this,ex]
            {
                QMessageBox::critical(
                    parent_,
                    "WSL backend update failed",
                    "The backend update failed.\n"
                    "Reason:\n"
                    +QString(ex.what()) );
            }
        );
    }
}

bool UpdateWSLVersionThread::success() const
{
    return success_;
}



UpdateWSLProgressDialog::UpdateWSLProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    auto vl = new QVBoxLayout;
    setLayout(vl);

    pte_=new QPlainTextEdit;
    vl->addWidget(pte_);

    auto hl=new QHBoxLayout;
    auto sp=new QSpacerItem(1, 1, QSizePolicy::Expanding);
    hl->addItem(sp);
    btn_=new QPushButton("Cancel");
    connect(btn_, &QPushButton::clicked,
            this, &QDialog::close);
    hl->addWidget(btn_);
    vl->addLayout(hl);
}

void UpdateWSLProgressDialog::appendLogLine(const QString &line)
{
    pte_->appendPlainText(line);
}

void UpdateWSLProgressDialog::changeCloseBtnText(const QString& label)
{
    btn_->setText("Close");
}

void checkExternalPrograms(ActionProgress& prog, QWidget *parent)
{
    auto mp = insight::ExternalPrograms::globalInstance()
                  .findMissingPrograms(prog);

    if (mp.size()>0)
    {
        QMetaObject::invokeMethod(
            qApp,
            [parent,mp](){
                QString msg="The following external programs have not been found:\n";
                for (const auto&p: mp)
                    msg+=QString::fromStdString(p)+"\n";
                msg+="Do you want to select the paths to these executables now?";

                auto answer=QMessageBox::question(
                    parent,
                    "Missing external programs",
                    msg,
                    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
                if (answer==QMessageBox::Yes)
                {
                    IQConfigureExternalProgramsDialog dlg(parent);
                    dlg.exec();
                }
            },
            Qt::QueuedConnection);
    }
}



} // namespace insight
