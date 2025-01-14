
#include <QAbstractItemModelTester>

#include "iqparametersetmodel.h"
#include "numericalwindtunnel.h"

using namespace insight;

int main(int argc, char*argv[])
{
    auto ps = NumericalWindtunnel::defaultParameters();
    IQParameterSetModel modelToBeTested(*ps, *NumericalWindtunnel::defaultParameters());
    QAbstractItemModelTester tester(&modelToBeTested, QAbstractItemModelTester::FailureReportingMode::Fatal);
}
