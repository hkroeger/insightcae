#include "iqselectremotehosttypedialog.h"
#include "ui_iqselectremotehosttypedialog.h"
#include "iqsetupwsldistributionwizard.h"

#include "base/sshlinuxserver.h"
#include "base/wsllinuxserver.h"

#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>


const int RST_SSHLINUX=0;
const int RST_WSLLINUX=1;


ServerSetup::ServerSetup(
    QWidget* parent,
    IQSelectRemoteHostTypeDialog *dlg )
  : parent_(parent), dlg_(dlg)
{}


ServerSetup::~ServerSetup()
{
  for (auto* w: parent_->children())
  {
    w->deleteLater();
  }
  delete parent_->layout();
}

QString ServerSetup::validationError() const
{
  return QString();
}


Ui::IQSelectRemoteHostTypeDialog *ServerSetup::dlgui()
{
    return dlg_->ui;
}



SSHLinuxSetup::SSHLinuxSetup(
    QWidget *parent,
    IQSelectRemoteHostTypeDialog *dlg,
    insight::RemoteServer::ConfigPtr initialcfg )
  : ServerSetup(parent, dlg)
{
  auto ini =
        std::dynamic_pointer_cast
         <insight::SSHLinuxServer::Config>(
          initialcfg);

  auto* vl = new QVBoxLayout(parent);
  parent->setLayout(vl);

  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("Host name:"));
    leHostName_ = new QLineEdit;
    if (ini)
      leHostName_->setText( QString::fromStdString(ini->hostName_) );
    l->addWidget(leHostName_);
    vl->addLayout(l);
  }

  {
      auto *l = new QHBoxLayout;
      l->addWidget(new QLabel("Number of processors:"));
      sbNp_ = new QSpinBox;
      sbNp_->setMinimum(1);
      if (ini)
          sbNp_->setValue( ini->np_ );
      l->addWidget(sbNp_);
      vl->addLayout(l);
  }

  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("Base directory:"));
    leBaseDir_ = new QLineEdit;
    if (ini)
      leBaseDir_->setText( QString::fromStdString(ini->defaultDirectory_.string()) );
    l->addWidget(leBaseDir_);
    vl->addLayout(l);
  }

  const QString placeholderHelp =
      "%{serverLabel} and %{host} are replaced by the server label and the host name.\n"
      "In server pools, %d is replaced by the instance number.";

  // hosts, which are launched on demand (e.g. virtual machines)
  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("Launch command:"));
    leLaunchCommand_ = new QLineEdit;
    leLaunchCommand_->setPlaceholderText("(none: host is always running)");
    leLaunchCommand_->setToolTip(
        "Executed locally, when the host is needed but not reachable.\n"
        +placeholderHelp );
    if (ini)
      leLaunchCommand_->setText( QString::fromStdString(ini->launchCommand_) );
    l->addWidget(leLaunchCommand_);
    vl->addLayout(l);
  }

  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("Shutdown command:"));
    leShutdownCommand_ = new QLineEdit;
    leShutdownCommand_->setPlaceholderText("(none)");
    leShutdownCommand_->setToolTip(
        "Executed locally to shut the host down.\n"
        +placeholderHelp );
    if (ini)
      leShutdownCommand_->setText( QString::fromStdString(ini->shutdownCommand_) );
    l->addWidget(leShutdownCommand_);
    vl->addLayout(l);
  }

  // instances, which have to be created and disposed (e.g. cloud instances)
  auto addCommandRow = [&](const QString& label, const QString& tooltip,
                           const std::string* initial, QLineEdit*& le)
  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel(label));
    le = new QLineEdit;
    le->setPlaceholderText("(none)");
    le->setToolTip(tooltip+"\n"+placeholderHelp);
    if (initial)
      le->setText( QString::fromStdString(*initial) );
    l->addWidget(le);
    vl->addLayout(l);
  };
  addCommandRow(
      "Creation command:",
      "Executed locally to create the instance, when it is needed but does not exist.\n"
      "Afterwards, the host is expected to start up (otherwise, the launch command is executed).",
      ini ? &ini->creationCommand_ : nullptr, leCreationCommand_ );
  addCommandRow(
      "Destruction command:",
      "Executed locally to delete the instance (after shutting it down, if it is running).",
      ini ? &ini->destructionCommand_ : nullptr, leDestructionCommand_ );
  addCommandRow(
      "Existence check command:",
      "Executed locally to check, if the instance exists:\n"
      "exit code 0 means, the instance exists, any other exit code means, it does not exist.",
      ini ? &ini->existenceCheckCommand_ : nullptr, leExistenceCheckCommand_ );

  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("Launch timeout:"));
    sbLaunchTimeout_ = new QSpinBox;
    sbLaunchTimeout_->setRange(0, 7200);
    sbLaunchTimeout_->setSuffix(" s");
    sbLaunchTimeout_->setToolTip(
        "Time to wait after the launch command, until the host is reachable." );
    sbLaunchTimeout_->setValue( int(
        (ini ? ini->launchTimeout_
             : insight::SSHLinuxServer::Config::defaultLaunchTimeout()).count() ) );
    l->addWidget(sbLaunchTimeout_);
    l->addStretch();
    vl->addLayout(l);
  }

  // the timeout is only relevant with a launch or creation command
  auto updateTimeoutEnabled = [this]()
  {
    sbLaunchTimeout_->setEnabled(
        !leLaunchCommand_->text().trimmed().isEmpty()
        || !leCreationCommand_->text().trimmed().isEmpty() );
  };
  QObject::connect(leLaunchCommand_, &QLineEdit::textChanged, leLaunchCommand_, updateTimeoutEnabled);
  QObject::connect(leCreationCommand_, &QLineEdit::textChanged, leCreationCommand_, updateTimeoutEnabled);
  updateTimeoutEnabled();
}


insight::RemoteServer::ConfigPtr SSHLinuxSetup::result()
{
  auto cfg = std::make_shared<insight::SSHLinuxServer::Config>(
        leBaseDir_->text().toStdString(),
        sbNp_->value(),
        leHostName_->text().toStdString(),
        leLaunchCommand_->text().trimmed().toStdString(),
        leShutdownCommand_->text().trimmed().toStdString()
        );
  cfg->creationCommand_ = leCreationCommand_->text().trimmed().toStdString();
  cfg->destructionCommand_ = leDestructionCommand_->text().trimmed().toStdString();
  cfg->existenceCheckCommand_ = leExistenceCheckCommand_->text().trimmed().toStdString();
  cfg->launchTimeout_ = std::chrono::seconds(sbLaunchTimeout_->value());
  return cfg;
}


QString SSHLinuxSetup::validationError() const
{
  // required by the server list file format
  if ( leLaunchCommand_->text().trimmed().isEmpty()
       != leShutdownCommand_->text().trimmed().isEmpty() )
  {
    return "Launch and shutdown command must be both given, or both left empty.";
  }

  int nCreationCommands =
      int(!leCreationCommand_->text().trimmed().isEmpty())
      + int(!leDestructionCommand_->text().trimmed().isEmpty())
      + int(!leExistenceCheckCommand_->text().trimmed().isEmpty());
  if (nCreationCommands!=0 && nCreationCommands!=3)
  {
    return "Creation, destruction and existence check command must be all given, or all left empty.";
  }
  return QString();
}





WSLLinuxSetup::WSLLinuxSetup(
    QWidget *parent,
    IQSelectRemoteHostTypeDialog *dlg,
    insight::RemoteServer::ConfigPtr initialcfg )
  : ServerSetup(parent, dlg)
{
  auto ini =
        std::dynamic_pointer_cast
         <insight::WSLLinuxServer::Config>(
          initialcfg);

  auto* vl = new QVBoxLayout(parent);
  parent->setLayout(vl);

#ifdef WIN32
  {
      auto* btn=new QPushButton("Set up new WSL distribution...");
      QObject::connect(btn, &QPushButton::clicked, btn,
              [&]()
              {
                  IQSetupWSLDistributionWizard wizdlg;
                  if (wizdlg.exec() == QDialog::Accepted)
                  {
                      insight::dbg()<<"accepted"<<std::endl;
                      dlgui()->leServerLabel->setText( wizdlg.distributionLabel() );
                      leDistributionLabel_->setCurrentText( wizdlg.distributionLabel() );
                      leBaseDir_->setText( wizdlg.baseDirectory() );

                      insight::dbg()<<"information"<<std::endl;
                      QMessageBox::information(
                                  parent_,
                                  "Success!",
                                  "The WSL distribution has been created\n"
                                  "and the configuration data has been copied\n"
                                  "into the corresponding input fields.\n\n"
                                  "You may simply click on \"Ok\" now\n"
                                  "to create a matching remote server entry!"
                                  );
                  }
              }
      );
      vl->addWidget(btn);
  }
#endif
  {
    auto *l = new QHBoxLayout;

    l->addWidget(new QLabel("WSL distribution label:"));
    leDistributionLabel_ = new QComboBox;

    auto distros=insight::WSLLinuxServer::listWSLDistributions();
    for (const auto& d: distros)
    {
        leDistributionLabel_->addItem(QString::fromStdString(d));
    }

    auto defdistro = insight::WSLLinuxServer::defaultWSLDistributionName();
    int idef=-1;
    auto itrdef=std::find(distros.begin(), distros.end(), defdistro);
    if (itrdef!=distros.end())
    {
        idef=std::distance(distros.begin(), itrdef);
        insight::dbg()<<"i def distro="<<idef<<std::endl;
    }


    leDistributionLabel_->setEditable(true);
    if (ini)
    {
        leDistributionLabel_->setCurrentText( QString::fromStdString(ini->distributionLabel_) );
    }
    else
    {
        if (idef>=0)
        {
            leDistributionLabel_->setCurrentIndex(idef);
        }
    }

    l->addWidget(leDistributionLabel_);

    vl->addLayout(l);
  }

  {
      auto *l = new QHBoxLayout;
      l->addWidget(new QLabel("Number of processors:"));
      sbNp_ = new QSpinBox;
      sbNp_->setMinimum(1);
      if (ini)
          sbNp_->setValue( ini->np_ );
      l->addWidget(sbNp_);
      vl->addLayout(l);
  }

  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("Base directory:"));
    leBaseDir_ = new QLineEdit;
    if (ini)
      leBaseDir_->setText( QString::fromStdString(ini->defaultDirectory_.string()) );
    l->addWidget(leBaseDir_);
    vl->addLayout(l);
  }
}




insight::RemoteServer::ConfigPtr WSLLinuxSetup::result()
{
  return std::make_shared<insight::WSLLinuxServer::Config>(
        leBaseDir_->text().toStdString(),
        sbNp_->value(),
        leDistributionLabel_->currentText().toStdString()
        );
}




IQSelectRemoteHostTypeDialog::IQSelectRemoteHostTypeDialog(
    const insight::RemoteServerList& remoteServers,
    insight::RemoteServer::ConfigPtr cfgToEdit,
    QWidget *parent )
  :
  QDialog(parent),
  remoteServers_(remoteServers),
  entryShouldNotExist_(!cfgToEdit),
  result_(cfgToEdit),
  ui(new Ui::IQSelectRemoteHostTypeDialog)
{
  ui->setupUi(this);

  auto *vali = new QRegExpValidator(QRegExp("[a-zA-Z][a-zA-Z0-9]*"), this);
  ui->leServerLabel->setValidator(vali);

  if (result_)
    ui->leServerLabel->setText(QString::fromStdString(*result_));


  auto onIndexChange = [&](int index)
  {
    setupControls_.reset(); // remove
    switch(index)
    {
      case RST_SSHLINUX: // SSH Linux
        setupControls_.reset(new SSHLinuxSetup(ui->gbServerSetup, this, result_));
        break;
      case RST_WSLLINUX: // WSL
        setupControls_.reset(new WSLLinuxSetup(ui->gbServerSetup, this, result_));
        break;
    }
  };

  connect(ui->cbServerType, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, onIndexChange);

  if (auto sshcfg = std::dynamic_pointer_cast<insight::SSHLinuxServer::Config>(cfgToEdit))
  {
    ui->cbServerType->setCurrentIndex(RST_SSHLINUX);
  }
  else if (auto wslcfg = std::dynamic_pointer_cast<insight::WSLLinuxServer::Config>(cfgToEdit))
  {
    ui->cbServerType->setCurrentIndex(RST_WSLLINUX);
  }

  onIndexChange(ui->cbServerType->currentIndex());
}




IQSelectRemoteHostTypeDialog::~IQSelectRemoteHostTypeDialog()
{
  delete ui;
}




void IQSelectRemoteHostTypeDialog::accept()
{
  std::string srvlbl = ui->leServerLabel->text().toStdString();

  if (srvlbl.empty())
  {
    QMessageBox::critical(
          this,
          "Invalid server label",
          "The server label must not be empty!");
    return;
  }

  bool exists=false;
  if (entryShouldNotExist_)
  {
    try
    {
      remoteServers_.findServer(srvlbl);
      exists=true;
    }  catch (insight::Exception& ex) {}
  }

  if (exists)
  {
    QMessageBox::critical(
          this,
          "Invalid server label",
          "The server label exists already!");
    return;
  }
  else
  {
    if (setupControls_)
    {
      auto err = setupControls_->validationError();
      if (!err.isEmpty())
      {
        QMessageBox::critical(this, "Invalid server configuration", err);
        return;
      }

      result_=setupControls_->result();
      static_cast<std::string&>(*result_) = srvlbl;
    }

    QDialog::accept();
  }
}





