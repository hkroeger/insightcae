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


ServerSetup::ServerSetup(QWidget* parent)
  : parent_(parent)
{}


ServerSetup::~ServerSetup()
{
  for (auto* w: parent_->children())
  {
    w->deleteLater();
  }
  delete parent_->layout();
}

Ui::IQSelectRemoteHostTypeDialog *ServerSetup::dlgui()
{
    auto p = dynamic_cast<IQSelectRemoteHostTypeDialog*>(parent_);
    return p->ui;
}



SSHLinuxSetup::SSHLinuxSetup(QWidget *parent, insight::RemoteServer::ConfigPtr initialcfg)
  : ServerSetup(parent)
{
  auto ini = std::dynamic_pointer_cast<insight::SSHLinuxServer::Config>(initialcfg);

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
    l->addWidget(new QLabel("Base directory:"));
    leBaseDir_ = new QLineEdit;
    if (ini)
      leBaseDir_->setText( QString::fromStdString(ini->defaultDirectory_.string()) );
    l->addWidget(leBaseDir_);
    vl->addLayout(l);
  }
}


insight::RemoteServer::ConfigPtr SSHLinuxSetup::result()
{
  return std::make_shared<insight::SSHLinuxServer::Config>(
        leBaseDir_->text().toStdString(),
        leHostName_->text().toStdString()
        );
}





WSLLinuxSetup::WSLLinuxSetup(QWidget *parent, insight::RemoteServer::ConfigPtr initialcfg)
  : ServerSetup(parent)
{
  auto ini = std::dynamic_pointer_cast<insight::WSLLinuxServer::Config>(initialcfg);

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
                      dlgui()->leServerLabel->setText( wizdlg.distributionLabel() );
                      leDistributionLabel_->setCurrentText( wizdlg.distributionLabel() );
                      leBaseDir_->setText( wizdlg.baseDirectory() );
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
        leDistributionLabel_->currentText().toStdString()
        );
}




IQSelectRemoteHostTypeDialog::IQSelectRemoteHostTypeDialog(
    insight::RemoteServerList& remoteServers,
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
        setupControls_.reset(new SSHLinuxSetup(ui->gbServerSetup, result_));
        break;
      case RST_WSLLINUX: // WSL
        setupControls_.reset(new WSLLinuxSetup(ui->gbServerSetup, result_));
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
      result_=setupControls_->result();
      static_cast<std::string&>(*result_) = srvlbl;
    }

    QDialog::accept();
  }
}





