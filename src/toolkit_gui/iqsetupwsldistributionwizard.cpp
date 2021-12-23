#include <QDebug>
#include <QMessageBox>
#include <QPushButton>

#include "iqsetupwsldistributionwizard.h"
#include "iqfiledownloader.h"
#include "ui_iqsetupwsldistributionwizard.h"

#include "base/exception.h"
#include "base/tools.h"
#include "boost/regex.hpp"
#include "base/wsllinuxserver.h"





//QString IQSetupWSLDistributionWizard::effectiveRepoURL() const
//{
//    QRegExp re("^(http|https):\\/\\/(.*@|)([^/]*)\\/(.*)$");
//    if (!re.exactMatch(ui->leRepoURL->text()))
//    {
//        return QString();
//    }

//    QString cred = re.cap(2);
//    if (!ui->leRepoUser->text().isEmpty())
//    {
//        cred = QString("%1:%2@").arg(
//                    ui->leRepoUser->text(),
//                    ui->leRepoPwd->text()
//                    );
//    }

//    return QString("%1://%2%3/%4").arg(
//                re.cap(1),
//                cred,
//                re.cap(3),
//                re.cap(4)
//                );
//}




QProcess* IQSetupWSLDistributionWizard::launchSubprocess(
        const QString& cmd,
        const QStringList& args,
        const QString& explainText,
        std::function<void()> nextStep
        )
{
    if (wanim_) wanim_->deleteLater();

    ui->statusText->setText(explainText);
    wanim_ = new IQWaitAnimation( ui->progress, 600000 );

    insight::dbg()<<"creating subprocess"<<std::endl;
    QString cmbargs;
    for (const auto& a: args) cmbargs+=" \""+a+"\"";
    insight::dbg()<<cmd.toStdString()<<cmbargs.toStdString()<<std::endl;

    auto proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyRead, proc,
            [this,proc]
            {
                ui->log->appendPlainText( proc->readAll() );
            }
    );
    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc,
            [this,proc,nextStep](int exitCode, QProcess::ExitStatus exitStatus)
            {
                insight::dbg()<<"subprocess finished"<<std::endl;
                if (wanim_) wanim_->deleteLater();

                if (exitCode==0 && exitStatus==QProcess::NormalExit)
                {
                    QMetaObject::invokeMethod(
                          qApp, nextStep );
                }
                else
                {
                    failed(
                        "Creation of the WSL distribution failed!\n"
                        "Please contact the support and provide a copy of the console log above!"
                        );
                }
                proc->deleteLater();
            }
    );

    insight::dbg()<<"launching subprocess"<<std::endl;
    proc->start( cmd, args );

    if (!proc->waitForStarted())
    {
        insight::dbg()<<"subprocess not started"<<std::endl;
        failed(proc->errorString());
        proc->deleteLater();
    }
    return proc;
}




void IQSetupWSLDistributionWizard::start()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->statusLabel->setEnabled(true);
    ui->progress->setEnabled(true);
    ui->statusText->setEnabled(true);
    ui->log->setEnabled(true);

//    downloadWSLImage();
    createWSLDistribution();
//    configureWSLDistribution();
//    restartWSLDistribution();
}




//void IQSetupWSLDistributionWizard::downloadWSLImage()
//{
//    insight::dbg()<<"downloadWSLImage"<<std::endl;

//    auto fdl = new IQFileDownloader(
//                QString::fromStdString(
//                    insight::TemporaryFile("ubuntu-rootfs-%%%%.tgz")
//                    .path().string() ),
//                this
//                );
//    connect(fdl, &IQFileDownloader::failed,
//            this, &IQSetupWSLDistributionWizard::failed);

//    fdl->connectProgressBar(ui->progress);
//    fdl->connectLabel(ui->statusText);
//    connect(fdl, &IQFileDownloader::finished,
//            this, &IQSetupWSLDistributionWizard::createWSLDistribution);
//    fdl->start(QUrl("http://downloads.silentdynamics.de/thirdparty/ubuntu-18.04-server-cloudimg-amd64-wsl.rootfs.tar.gz"));
//}




void IQSetupWSLDistributionWizard::createWSLDistribution()
{
    insight::dbg()<<"createWSLDistribution"<<std::endl;

    auto imageFile =
        insight::SharedPathList().getSharedFilePath(
                boost::filesystem::path("wsl")/(distributionLabel().toStdString()+".tar.gz") );

    insight::dbg() << "image file = " << imageFile << std::endl;

    boost::filesystem::path targpath =
            QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation )
            .toStdString();
    targpath = targpath / distributionLabel().toStdString();

    insight::dbg()<<targpath<<std::endl;

    launchSubprocess(
        wslexe_,
        {  "--import",
           distributionLabel(),
           QString::fromStdString( targpath.string() ),
           QString::fromStdString( imageFile.string() ),
           "--version", "1" },

        "Importing the WSL backend distribution. This will take several minutes",

        std::bind(&IQSetupWSLDistributionWizard::restartWSLDistribution /*configureWSLDistribution*/, this)
    );
}




//void IQSetupWSLDistributionWizard::configureWSLDistribution()
//{
//    insight::dbg()<<"configureWSLDistribution"<<std::endl;

//    auto scriptfile = createSetupScript();

//    QFileInfo fn( scriptfile->fileName() );
//    launchSubprocess(
//        wslexe_,
//        { "-d", distributionLabel(),
//          "--cd", fn.path(),
//          "bash", "./"+fn.fileName() },

//        "Configuring the WSL backend distribution. This will take several minutes",

//        std::bind(&IQSetupWSLDistributionWizard::restartWSLDistribution, this)
//    );
//}




void IQSetupWSLDistributionWizard::restartWSLDistribution()
{
    launchSubprocess(
        wslexe_,
        { "--shutdown" },

        "Restarting WSL.",

        std::bind(&IQSetupWSLDistributionWizard::completed, this)
    );
}




void IQSetupWSLDistributionWizard::completed()
{
    QDialog::accept();
}



void IQSetupWSLDistributionWizard::failed(const QString &errorMsg)
{
    QMessageBox::critical(
                this, "Failed",
                errorMsg);
}




//QTemporaryFile* IQSetupWSLDistributionWizard::createSetupScript()
//{
//    if (ui->leWSLUser->text().isEmpty())
//    {
//        throw insight::Exception("WSL user name must not be empty!");
//    }
//    auto f = new QTemporaryFile(QDir::tempPath()+"/setupWSL-XXXXXX.sh", this);
//    if (f->open())
//    {
//        QString script =
//"#!/bin/bash\n"

//"UNAME="+ui->leWSLUser->text()+"\n"

//"apt-get update\n"
//"apt-get install -y ca-certificates sudo\n"
//"apt-key adv --fetch-keys http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg\n"
//"add-apt-repository "+ effectiveRepoURL() +"\n"
//"apt-get update\n"
//"apt-get install -y "+ QString::fromStdString(insight::WSLLinuxServer::installationPackageName()) +"\n"

//"useradd -m $UNAME\n"
//"echo \"$UNAME      ALL=(ALL) NOPASSWD:ALL\" >> /etc/sudoers\n"
//"sed -i \"1i source /opt/insightcae/bin/insight_setenv.sh\" /home/$UNAME/.bashrc\n"

//"if [ ! -d \""+baseDirectory()+"\"]; then\n"
//" mkdir \""+baseDirectory()+"\"\n"
//" chown $UNAME \""+baseDirectory()+"\"\n"
//"fi\n"

//"chsh -s /bin/bash $UNAME\n"

//"cat > /etc/wsl.conf << EOF\n"
//"[user]\n"
//"default=$UNAME\n"
//"EOF\n"
//              ;

//        insight::dbg() << script.toStdString() << std::endl;

//        QTextStream out(f);
//        out << script;
//        out.flush();
//        f->close();
//    }

//    return f;
//}



IQSetupWSLDistributionWizard::IQSetupWSLDistributionWizard(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IQSetupWSLDistributionWizard)
{
    ui->setupUi(this);

//    auto defwsllabel=insight::WSLLinuxServer::defaultWSLDistributionName();
//    auto lbl=defwsllabel;
//    auto distros=insight::WSLLinuxServer::listWSLDistributions();
//    int maxatt=99;
//    for (int attmpt=1; attmpt<maxatt; ++attmpt)
//    {
//        if (std::find(distros.begin(), distros.end(), lbl)!=distros.end())
//        {
//            lbl=str(boost::format("%s_%d")%defwsllabel%attmpt);
//        }
//        else
//            break;
//    }

    auto distros=insight::WSLLinuxServer::listWSLDistributions();
    auto lbl = insight::findUnusedLabel(
                distros.begin(), distros.end(),
                insight::WSLLinuxServer::defaultWSLDistributionName()
                );
    ui->leWSLLabel->setText(QString::fromStdString(lbl));
}




IQSetupWSLDistributionWizard::~IQSetupWSLDistributionWizard()
{
    delete ui;
}




void IQSetupWSLDistributionWizard::accept()
{
    start();
}




QString IQSetupWSLDistributionWizard::distributionLabel() const
{
    return ui->leWSLLabel->text();
}




QString IQSetupWSLDistributionWizard::baseDirectory() const
{
    return QString("/home/user");
}
