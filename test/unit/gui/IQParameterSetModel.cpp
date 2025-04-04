
#include <QAbstractItemModelTester>

#include "iqparametersetmodel.h"
#include "numericalwindtunnel.h"

using namespace insight;

int main(int argc, char*argv[])
{
    IQParameterSetModel modelToBeTested(NumericalWindtunnel::defaultParameters());
    QAbstractItemModelTester tester(&modelToBeTested, QAbstractItemModelTester::FailureReportingMode::Fatal);
}
