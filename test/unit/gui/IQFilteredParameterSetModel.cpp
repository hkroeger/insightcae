
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QDialog>
#include <QTreeView>
#include <QVBoxLayout>

#include "iqparametersetmodel.h"
#include "sampleparameterset.h"

using namespace insight;

int main(int argc, char*argv[])
{
    ParameterSet ps(TestPS::defaultParameters());

    IQParameterSetModel baseForModelToBeTested(ps, TestPS::defaultParameters());

    IQFilteredParameterSetModel modelToBeTested({"mesh"});
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
