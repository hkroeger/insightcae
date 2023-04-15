
#include "iqcaditemmodel.h"
#include "iqcadmodel3dviewer.h"

#include "datum.h"
#include "iqvtkcadmodel3dviewer.h"

#include <QInputDialog>
#include <QColorDialog>
#include <QFileDialog>

void IQCADItemModel::showContextMenu(const QModelIndex &idx, const QPoint &pos, IQCADModel3DViewer* viewer)
{
    QMenu cm;

    QAction *a;

    if (!idx.parent().isValid()) // on top level node in tree view
    {
        if (idx.row()==CADModelSection::datum)
        {
            a=new QAction("Create plane...", &cm);
            connect(a, &QAction::triggered,
                    this, &IQCADItemModel::addPlane );
            cm.addAction(a);
        }

        else if (idx.row()==CADModelSection::feature)
        {
            a=new QAction("Import model...", &cm);
            connect(a, &QAction::triggered,
                    this, &IQCADItemModel::addImportedFeature );
            cm.addAction(a);
        }
    }



    else // on item



    {

        if (idx.internalId()==CADModelSection::feature)
        {
            auto feat = data(idx.siblingAtColumn(IQCADItemModel::entityCol)).value<insight::cad::FeaturePtr>();
            auto name = QString::fromStdString(feat->featureSymbolName());
            a=new QAction(name + ": Jump to Def.", &cm);
            connect(a, &QAction::triggered,
                    std::bind(&IQCADItemModel::jumpToDefinition, this, name) );
            cm.addAction(a);

            a=new QAction("Insert name", &cm);
            connect(a, &QAction::triggered,
                    std::bind(&IQCADItemModel::insertParserStatementAtCursor, this, name) );
            cm.addAction(a);

            if (viewer)
            {
                if (auto psk = std::dynamic_pointer_cast<insight::cad::ConstrainedSketch>(feat))
                {
                    a=new QAction("Edit sketch...", &cm);
                    connect(a, &QAction::triggered,
                            std::bind(
                                &IQCADModel3DViewer::editSketch,
                                viewer, name.toStdString(), psk,
                                insight::ParameterSet(),
                                [](const insight::ParameterSet&, vtkProperty* actprops)
                                {
                                    actprops->SetColor(1,0,0);
                                    actprops->SetLineWidth(2);
                                }) );
                    cm.addAction(a);
                }
            }
        }

        a=new QAction("Show", &cm);
        connect(a, &QAction::triggered,
                [this,idx]() {
                    QModelIndex visi=index(idx.row(), IQCADItemModel::visibilityCol, idx.parent());
                    if (flags(visi)&Qt::ItemIsUserCheckable)
                    {
                        setData(visi, Qt::Checked, Qt::CheckStateRole);
                    }
                });
        cm.addAction(a);

        a=new QAction("Hide", &cm);
        connect(a, &QAction::triggered,
                [this,idx]() {
                    QModelIndex visi=index(idx.row(), IQCADItemModel::visibilityCol, idx.parent());
                    if (flags(visi)&Qt::ItemIsUserCheckable)
                    {
                        setData(visi, Qt::Unchecked, Qt::CheckStateRole);
                    }
                });
        cm.addAction(a);

        if (viewer)
        {
            cm.addSeparator();

            a = new QAction(("Fit &all"), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::fitAll );

            QMenu* directionmenu=cm.addMenu("Standard views");
            directionmenu->addAction( a = new QAction(("+X"), this) );
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::viewFront);
            directionmenu->addAction( a = new QAction(("-X"), this) );
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::viewBack);

            directionmenu->addAction( a = new QAction(("+Y"), this) );
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::viewRight);
            directionmenu->addAction( a = new QAction(("-Y"), this) );
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::viewLeft);

            directionmenu->addAction( a = new QAction(("+Z"), this) );
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::viewBottom);
            directionmenu->addAction( a = new QAction(("-Z"), this) );
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::viewTop);



            a = new QAction(("Toggle clip plane (&XY)"), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::toggleClipXY);
            a = new QAction(("Toggle clip plane (&YZ)"), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::toggleClipYZ);
            a = new QAction(("Toggle clip plane (X&Z)"), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::toggleClipXZ);

            QMenu *msmenu=cm.addMenu("Measure");

            a=new QAction("Distance between points", this);
            msmenu->addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::onMeasureDistance);

            a=new QAction("Select vertices", this);
            msmenu->addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::onSelectPoints);

            a=new QAction("Select edges", this);
            msmenu->addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::onSelectEdges);

            a=new QAction("Select faces", this);
            msmenu->addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::onSelectFaces);


            a = new QAction(("Change background color..."), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::selectBackgroundColor );

            cm.addSeparator();

            a = new QAction(("Only this shaded"), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    std::bind(&IQCADModel3DViewer::onlyOneShaded, viewer,
                              QPersistentModelIndex(idx) ) );

            a = new QAction(("Reset shading"), this);
            cm.addAction(a);
            connect(a, &QAction::triggered,
                    viewer, &IQCADModel3DViewer::resetRepresentations );
        }

        cm.addSeparator();

        if (idx.internalId()==CADModelSection::datum)
        {
            auto datum = data(idx.siblingAtColumn(IQCADItemModel::entityCol))
                    .value<insight::cad::DatumPtr>();

            if (datum->providesPlanarReference())
            {
                a = new QAction(("Create sketch on this plane..."), this);
                cm.addAction(a);
                connect(a, &QAction::triggered,
                        std::bind(&IQCADModel3DViewer::doSketchOnPlane, viewer, datum) );

                a = new QAction(("Import sketch on this plane..."), this);
                cm.addAction(a);
                connect(a, &QAction::triggered,
                        std::bind(&IQCADItemModel::addImportedSketch, this, datum) );
            }

    //        auto datums = model_->datums();
    //        auto i=datums.begin();
    //        std::advance(i, index.row());
    //        datumVisibility_[i->second] =
    //                ( value.value<Qt::CheckState>()==Qt::Checked );
    //        Q_EMIT dataChanged(index, index, {role});
    //        return true;
        }
        else if (idx.internalId()==CADModelSection::feature)
        {
            a=new QAction("Shaded", &cm);
            connect(a, &QAction::triggered,
                    [this,idx]() {
                        QModelIndex featrepr=index(idx.row(), IQCADItemModel::entityRepresentationCol, idx.parent());
                        setData(featrepr, insight::DatasetRepresentation::Surface, Qt::EditRole);
                    });
            cm.addAction(a);

            a=new QAction("Wireframe", &cm);
            connect(a, &QAction::triggered,
                    [this,idx]() {
                        QModelIndex featrepr=index(idx.row(), IQCADItemModel::entityRepresentationCol, idx.parent());
                        setData(featrepr, insight::DatasetRepresentation::Wireframe, Qt::EditRole);
                    });
            cm.addAction(a);

            a=new QAction("Set opacity...", &cm);
            connect(a, &QAction::triggered, this,
                    [this,idx]() {
                        bool ok=false;
                        double val=QInputDialog::getDouble(nullptr, "opacity", "opacity", 1, 0, 1, 2, &ok);
                        if (ok)
                        {
                            QModelIndex visi=index(idx.row(), IQCADItemModel::entityOpacityCol, idx.parent());
                            setData(visi, val, Qt::EditRole);
                        }
                    });
            cm.addAction(a);

            a=new QAction("Set color...", &cm);
            connect(a, &QAction::triggered, this,
                    [this,idx]() {
                        bool ok=false;
                        QModelIndex ci=index(idx.row(), IQCADItemModel::entityColorCol, idx.parent());
                        auto val=QColorDialog::getColor(
                                    data(ci).value<QColor>(),
                                    nullptr,
                                    "Set Color");
                        if (val.isValid())
                        {
                            setData(ci, val, Qt::EditRole);
                        }
                    });
            cm.addAction(a);

            auto feat = data(idx.siblingAtColumn(IQCADItemModel::entityCol))
                    .value<insight::cad::FeaturePtr>();

            bool someSubMenu=false, someHoverDisplay=false;
            addSymbolsToSubmenu(
                        QString::fromStdString(feat->featureSymbolName()),
                        &cm, feat,
                        &someSubMenu, &someHoverDisplay);
            if (someHoverDisplay)
            {
              connect(&cm, &QMenu::aboutToHide,
                    this, &IQCADItemModel::undoHighlightInView);
            }
        }
        else if (idx.internalId()==CADModelSection::dataset)
        {
            a=new QAction("Set representation...", &cm);
            connect(a, &QAction::triggered, this,
                    [this,idx]() {
                        bool ok=false;
                        auto n=QInputDialog::getItem(nullptr,
                                                     "repr", "representation",
                                                     {"Points", "Wireframe", "Surface"},
                                                     2, false, &ok);
                        if (ok)
                        {
                            int i;
                            if (n=="Points")
                                i=VTK_POINTS;
                            else if (n=="Wireframe")
                                i=VTK_WIREFRAME;
                            else if (n=="Surface")
                                i=VTK_SURFACE;

                                 QModelIndex visi=index(idx.row(), IQCADItemModel::datasetRepresentationCol, idx.parent());
                                 setData(visi, i, Qt::EditRole);
                        }
                    });
            cm.addAction(a);

            a=new QAction("Auto scale set data range", &cm);
            connect(a, &QAction::triggered, this,
                    [this,idx]() {
                         QModelIndex visi=index(idx.row(), IQCADItemModel::datasetMinCol, idx.parent());
                         setData(visi, QVariant(), Qt::EditRole);
                         visi=index(idx.row(), IQCADItemModel::datasetMaxCol, idx.parent());
                         setData(visi, QVariant(), Qt::EditRole);
                    });
            cm.addAction(a);
            a=new QAction("Manual set data range minimum", &cm);
            connect(a, &QAction::triggered, this,
                    [this,idx]() {
                        bool ok=false;
                        double val=QInputDialog::getDouble(nullptr, "minimum", "minimum", 0, -DBL_MAX, DBL_MAX, 1, &ok);
                        if (ok)
                        {
                            QModelIndex visi=index(idx.row(), IQCADItemModel::datasetMinCol, idx.parent());
                            setData(visi, val, Qt::EditRole);
                        }
                    });
            cm.addAction(a);
            a=new QAction("Manual set data range maximum", &cm);
            connect(a, &QAction::triggered, this,
                    [this,idx]() {
                        bool ok=false;
                        double val=QInputDialog::getDouble(nullptr, "maximum", "maximum", 0, -DBL_MAX, DBL_MAX, 1, &ok);
                        if (ok)
                        {
                            QModelIndex visi=index(idx.row(), IQCADItemModel::datasetMaxCol, idx.parent());
                            setData(visi, val, Qt::EditRole);
                        }
                    });
            cm.addAction(a);

        }
    }

    cm.exec(pos);
}
