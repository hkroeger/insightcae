#include "insertdrawingdialog.h"
#include "ui_insertdrawingdialog.h"

#include "qtextensions.h"
#include "base/translations.h"
#include "viewdefinitiondialog.h"
#include <iterator>
#include <qbuttongroup.h>

InsertDrawingDialog::InsertDrawingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InsertDrawingDialog)
{
    ui->setupUi(this);

    connect(
        ui->selectFileBtn, &QPushButton::clicked, this,
        [this]()
        {
            if (auto fn=getFileName(
                this, _("Select output file name"),
                GetFileMode::Save,
                {{"dxf", "Drawing exchange format DXF"}} ))
            {
                ui->outFileName->setText(fn);
            }
        }
    );

    ui->viewsList->setModel(&views_);

    connect(
        ui->addViewBtn, &QPushButton::clicked, this,
        [this]()
        {
            ViewDefinitionDialog dlg(this);
            if (dlg.exec()==QDialog::Accepted)
            {
                views_.appendView(dlg.viewDef());
            }
        }
    );

    connect(
        ui->editViewBtn, &QPushButton::clicked, this,
        [this]()
        {
            auto i=ui->viewsList->currentIndex();
            if (i.isValid())
            {
                auto vd=views_.view(i);
                ViewDefinitionDialog dlg(this, vd);
                if (dlg.exec()==QDialog::Accepted)
                {
                    views_.editView(i, dlg.viewDef());
                }
            }
        }
        );

    connect(
        ui->delViewBtn, &QPushButton::clicked, this,
        [this]()
        {
            auto i=ui->viewsList->currentIndex();
            if (i.isValid())
            {
                views_.removeView(i);
            }
        }
        );

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &InsertDrawingDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &InsertDrawingDialog::reject);
}

InsertDrawingDialog::~InsertDrawingDialog()
{
    delete ui;
}

void InsertDrawingDialog::accept()
{
    expression_=
      "DXF(\""+ui->outFileName->text().toStdString()+"\")"
      " << "+ui->featSymbol->text().toStdString() +"\n";

    std::vector<std::string> viewDefExprs;
    std::transform(
        views_.viewDefinitions().begin(), views_.viewDefinitions().end(),
        std::back_inserter(viewDefExprs),
        [](const DrawingViewDefinition& d)
        {
            return
                d.label+" "
                   +generateViewDefinitionExpression(d);
        });

    expression_+= boost::join(viewDefExprs, ",\n");

    expression_ += ";";

    QDialog::accept();
}

std::string InsertDrawingDialog::expression() const
{
    return expression_;
}
