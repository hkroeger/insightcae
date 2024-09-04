
#include "iqcaditemmodel.h"
#include "cadfeature.h"
#include "datum.h"
#include "sketch.h"

#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>


#include "dialogs/defineplanedialog.h"

#include "cadfeatures/importsolidmodel.h"

using namespace insight;

void IQCADItemModel::addPlane()
{
    DefinePlaneDialog dlg;
    if (dlg.exec()==QDialog::Accepted)
    {
        this->addDatum(
                    dlg.label(),
                    std::make_shared<cad::DatumPlane>(
                        cad::matconst(dlg.p0()),
                        cad::matconst(dlg.n())
                        )
                    );
    }
}



void IQCADItemModel::addImportedFeature()
{
    auto fn = QFileDialog::getOpenFileName(
                nullptr, "Select CAD model",
                "", "STEP (*.stp *.step);;IGES (*.igs *.iges);;Triangulated Surface (*.stl *.stlb);;OpenCASCADE BRep (*.brep)");
    if (!fn.isEmpty())
    {
        boost::filesystem::path fp(fn.toStdString());
        auto m = cad::Import::create(fp);
        this->addModelstep(
                    fp.filename().stem().string(),
                    m, false );
    }
}




void IQCADItemModel::addImportedSketch(insight::cad::DatumPtr plane)
{
    QFileDialog fndlg;
    fndlg.setWindowTitle("Select sketch file");

    QStringList filters = {"DXF (*.dxf)", "FreeCAD Sketch (*.fcstd *.FCStd)" };
    fndlg.setNameFilters(filters);

    auto* lab = new QLabel("Layer name:");
    auto* le = new QLineEdit;
    le->setText("0");
    auto *lay = static_cast<QGridLayout*>(fndlg.layout());
    int last_row=lay->rowCount(); // id of new row below
    lay->addWidget(lab, last_row, 0, 1, -1);
    lay->addWidget(le, last_row, 1, 1, -1);

    if (fndlg.exec()==QDialog::Accepted)
    {
        boost::filesystem::path fp(
                    fndlg.selectedFiles()[0].toStdString());

        this->addModelstep(
                    fp.filename().stem().string(),
                    cad::Sketch::create(
                        plane, fp, le->text().toStdString()
                        ), false );
    }
}
