#include "iqconfigureexternalprogramsdialog.h"
#include "ui_iqconfigureexternalprogramsdialog.h"
#include "qtextensions.h"

#include <QFileDialog>
#include <QMessageBox>

IQConfigureExternalProgramsDialog::IQConfigureExternalProgramsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IQConfigureExternalProgramsDialog)
{
    ui->setupUi(this);
    ui->programsList->setModel(&model_);
    ui->programsList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->programsList->setAlternatingRowColors(true);

    connect(ui->btnSelect, &QPushButton::clicked, this,
            [&]()
    {
        auto ci = ui->programsList->currentIndex();
        auto ep = model_.externalProgram( ci );
        if (ep!=insight::ExternalPrograms::iterator())
        {
            if (auto newpath = getFileName(
                    this,
                    "Please select program "+QString::fromStdString(ep->first),
                    GetFileMode::Open,
                    {
#ifdef WIN32
                        {"exe", "Executables"}
#else
                        {"*", "Executables"}
#endif
                    },
                    ep->second.parent_path()
                ))
            {
                model_.resetProgramPath(ci, newpath);
            }
        }
    }
    );
}

IQConfigureExternalProgramsDialog::~IQConfigureExternalProgramsDialog()
{
    delete ui;
}

const insight::ExternalPrograms &IQConfigureExternalProgramsDialog::externalProgams() const
{
    return model_.externalPrograms();
}

void IQConfigureExternalProgramsDialog::accept()
{
    auto listfile = insight::ExternalPrograms::globalInstance().firstWritableLocation();

    if (!listfile.empty())
    {
        auto decision=QMessageBox::question(
              this,
              "Confirm writing",
              QString(boost::filesystem::exists(listfile) ?
                "Do you want to overwrite" : "Do you want to create")
              +" the file "+QString::fromStdString(listfile.string())+"?",
              QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

        if ( decision == QMessageBox::Yes )
        {
          insight::ExternalPrograms::globalInstance() = model_.externalPrograms();
          insight::ExternalPrograms::globalInstance().writeConfiguration(listfile);
          QDialog::accept();
          return;
        }
        else if ( decision == QMessageBox::Cancel )
        {
          return;
        }
    }

    QMessageBox::critical(
          this,
          "Problem",
          "No suitable file location for the editied external programs list was found or accepted.\n"
          "The configuration could not be saved.",
          QMessageBox::Ok
          );
}
