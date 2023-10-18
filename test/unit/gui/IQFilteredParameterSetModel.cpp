
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QDialog>
#include <QTreeView>
#include <QVBoxLayout>

#include "iqparametersetmodel.h"
#include "numericalwindtunnel.h"

using namespace insight;

int main(int argc, char*argv[])
{
    ParameterSet ps(NumericalWindtunnel::defaultParameters());

    IQParameterSetModel baseForModelToBeTested(ps, NumericalWindtunnel::defaultParameters());

    IQFilteredParameterSetModel modelToBeTested({"geometry"});
    modelToBeTested.setSourceModel(&baseForModelToBeTested);

    QAbstractItemModelTester tester(&modelToBeTested, QAbstractItemModelTester::FailureReportingMode::Fatal);


    if (argc<=1 || std::string(argv[1])!="nogui")
    {
        QApplication app(argc, argv);
        QDialog dlg;
        auto *l=new QVBoxLayout;
        auto *tv=new QTreeView;
        l->addWidget(tv);
        tv->setModel(&modelToBeTested);
        dlg.setLayout(l);
        dlg.exec();
    }
}
