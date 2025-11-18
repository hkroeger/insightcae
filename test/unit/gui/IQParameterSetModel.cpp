
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QTreeView>
#include <QDialog>
#include <QHeaderView>

#include "iqparametersetmodel.h"
#include "test_pdl.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {
        auto tps = TestPDL::defaultParameters();
        auto &sk=tps->get<CADSketchParameter>("sketch");


        IQParameterSetModel modelToBeTested( std::move(tps) );

        QAbstractItemModelTester tester(
            &modelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal);

        if (argc<=1 || std::string(argv[1])!="nogui")
        {
            QApplication app(argc, argv);
            QDialog dlg;
            auto *l=new QVBoxLayout;
            auto *tv=new QTreeView;
            tv->setItemDelegate(new IQHierarchicalDataGridViewSelectorDelegate);
            l->addWidget(tv);
            tv->setModel(&modelToBeTested);
            tv->setContextMenuPolicy(Qt::CustomContextMenu);
            tv->setAlternatingRowColors(true);
            tv->setDragDropMode(QAbstractItemView::DragDrop);
            tv->setDefaultDropAction(Qt::MoveAction);
            tv->header()->setSectionResizeMode(
                QHeaderView::ResizeMode::ResizeToContents);
            tv->expandAll();
            QObject::connect(
                tv, &QTreeView::customContextMenuRequested,
                [tv](const QPoint& p)
                {
                    IQParameterSetModel::contextMenu(
                        tv,
                        tv->indexAt(p),
                        p );
                }
                );
            dlg.setLayout(l);

            sk.setScript(
                "layer standard"
                "SketchPoint( 0, 0.277723, 0.197252, layer standard),"
                "SketchPoint( 1, -0.401374, 0.523062, layer standard),"
                "SketchPoint( 2, 0.277723, 0.523062, layer standard),"
                "SketchPoint( 3, -0.401374, 0.197252, layer standard),"
                "Line(4, 1, 2, layer standard),"
                "Line(5, 2, 0, layer standard),"
                "Line(6, 0, 3, layer standard),"
                "Line(7, 3, 1, layer standard),"
                "FixedPoint( 8, 1, layer standard),"
                "DistanceConstraint( 9, 1, 2, layer standard),"
                "DistanceConstraint( 10, 2, 0, layer standard),"
                "HorizontalConstraint( 11, 4, layer standard),"
                "HorizontalConstraint( 12, 6, layer standard),"
                "VerticalConstraint( 13, 5, layer standard),"
                "VerticalConstraint( 14, 7, layer standard)"
                );
            sk.sketch();


            dlg.exec();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<"Failed: "<<ex.what()<<std::endl;
        return -1;
    }
    std::cout<<"Passed"<<std::endl;
    return 0;
}
