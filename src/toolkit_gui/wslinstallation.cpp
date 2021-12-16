#include "wslinstallation.h"

#include "base/tools.h"
#include "base/remoteserverlist.h"
#include "iqsetupwsldistributionwizard.h"
#include "iqremoteservereditdialog.h"

#include <QMessageBox>
#include <QPushButton>

namespace insight {


void updateWSLVersion(
        std::shared_ptr<insight::WSLLinuxServer> wsl,
        QWidget *parent )
{
    std::thread t(
        [parent,wsl]()
        {
            //          parent->statusBar()->showMessage("Update of WSL backend instance is running.");
            wsl->updateInstallation();
            QMetaObject::invokeMethod(
                        qApp,
                        [parent]
                        {
                            QMessageBox::information(
                                parent,
                                "WSL backend updated.",
                                "The backend update finished without error." );
                        }
            );
        }
    );
}



void checkWSLVersions(bool reportSummary, QWidget *parent)
{
  bool anythingChecked=false, anythingOutdated=false;

  for (auto& rs: insight::remoteServers)
  {
    if ( auto wslcfg = std::dynamic_pointer_cast<insight::WSLLinuxServer::Config>(rs) )
    {
      // check installed version inside WSL distribution
      if (auto wsl = std::dynamic_pointer_cast<insight::WSLLinuxServer>(wslcfg->instance()))
      {
        try
        {
          auto wslVersion = wsl->checkInstalledVersion();
          anythingChecked=true;
          if ( !(wslVersion == insight::ToolkitVersion::current()) )
          {
            anythingOutdated=true;
            auto answer = QMessageBox::warning(
                  parent,
                  "Inconsistent configuration",
                  "The backend package in the WSL environment is not of the same version as this GUI frontend.\n"
                  "(GUI version: "+QString::fromStdString(insight::ToolkitVersion::current().toString())+","
                  " WSL version: "+QString::fromStdString(wslVersion.toString())+")\n"
                  "Please check the reason. If you recently updated the GUI package, please update the backend before executing analyses.\n"
                  "\nExecute the backend update now?",
                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
                  );
            if (answer==QMessageBox::Yes)
            {
              updateWSLVersion( wsl, parent );
            }
          }
        }
        catch (const insight::Exception& ex)
        {
          QMessageBox::warning(
                parent,
                "Inconsistent configuration",
                "Installed backend version could not be checked.\n"+QString::fromStdString(ex.message())
                );
        }
      }
    }
  }

  if (!anythingChecked)
  {
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
              auto wslcfg = std::make_shared<insight::WSLLinuxServer::Config>(
                    wizdlg.baseDirectory().toStdString(),
                    wizdlg.distributionLabel().toStdString()
                    );

              insight::remoteServers.insert(wslcfg);

              insight::SharedPathList paths;
              for ( const bfs_path& p: paths )
              {
                  if ( exists(p) && is_directory ( p ) )
                  {
                    if ( insight::directoryIsWritable(p) )
                    {
                      bfs_path serverlist = bfs_path(p) / "remoteservers.list";

                      insight::remoteServers.writeConfiguration(serverlist);
                      break;
                    }
                  }
              }
          }
      }
      else if (qb.clickedButton()==selectBtn)
      {
          IQRemoteServerEditDialog dlg(parent);
          dlg.exec();
      }
  }
  else
  {
      if (reportSummary)
      {
          if (!anythingOutdated)
          {
              QMessageBox::information(
                          parent,
                          "WSL backend version check",
                          "All backend versions are correct!");
          }
      }
  }
}




} // namespace insight
