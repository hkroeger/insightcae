#ifndef IQSETUPWSLDISTRIBUTIONWIZARD_H
#define IQSETUPWSLDISTRIBUTIONWIZARD_H

#include <QDialog>

#include <QtNetwork>
#include <QProgressBar>
#include <QLabel>

#include "iqwaitanimation.h"


namespace Ui {
class IQSetupWSLDistributionWizard;
}




class IQSetupWSLDistributionWizard : public QDialog
{
    Q_OBJECT

    QString effectiveRepoURL() const;

    QPointer<IQWaitAnimation> wanim_;

    const QString wslexe_ = "c:\\windows\\sysnative\\wsl.exe";

    QProcess* launchSubprocess(
            const QString& cmd,
            const QStringList& args,
            const QString& explainText,
            std::function<void()> nextStep
            );

private Q_SLOTS:
    void start();
    void downloadWSLImage();
    void createWSLDistribution(const QString& imagefile);
    void configureWSLDistribution();
    void restartWSLDistribution();
    void completed();

    void failed(const QString& errorMsg);

    QTemporaryFile* createSetupScript();

public:
    explicit IQSetupWSLDistributionWizard(QWidget *parent = nullptr);
    ~IQSetupWSLDistributionWizard();

    void accept() override;

    QString distributionLabel() const;
    QString baseDirectory() const;

private:
    Ui::IQSetupWSLDistributionWizard *ui;
};




#endif // IQSETUPWSLDISTRIBUTIONWIZARD_H
