
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QTreeView>
#include <QDialog>

#include "iqparametersetmodel.h"
#include "numericalwindtunnel.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {
        IQParameterSetModel modelToBeTested(
            NumericalWindtunnel::defaultParameters());

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
