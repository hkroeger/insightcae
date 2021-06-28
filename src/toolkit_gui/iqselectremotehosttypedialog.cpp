#include "iqselectremotehosttypedialog.h"
#include "ui_iqselectremotehosttypedialog.h"

#include "base/sshlinuxserver.h"
#include "base/wsllinuxserver.h"

#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>


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

  {
    auto *l = new QHBoxLayout;
    l->addWidget(new QLabel("WSL executable:"));
    leWSLExecutable_ = new QLineEdit;
    if (ini)
      leWSLExecutable_->setText( QString::fromStdString(ini->WSLExecutable_.string()) );
    l->addWidget(leWSLExecutable_);
    auto* btn=new QPushButton("...");
    QObject::connect(btn, &QPushButton::clicked, btn,
            [&]()
            {
              auto sf = QFileDialog::getOpenFileName(
                    parent_,
                    "Select WSL executable",
                    "", "Executables (*.exe)");
              if (!sf.isEmpty())
              {
                leWSLExecutable_->setText(sf);
              }
            }
    );
    l->addWidget(btn);
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
        leWSLExecutable_->text().toStdString()
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





