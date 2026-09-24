
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTreeView>
#include <QDialog>
#include <QTimer>
#include <QHeaderView>
#include <QVBoxLayout>

#include <cmath>
#include <stdexcept>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "base/exception.h"
#include "iqparametersetmodel.h"
#include "test_pdl.h"

using namespace insight;

int main(int argc, char* argv[])
{
    // Parse before QApplication (which may modify argc/argv).
    bool nogui = (argc > 1 && std::string(argv[1]) == "nogui");

    // QApplication is required for QObject / signal infrastructure and
    // QSignalSpy.  In nogui / CI mode QT_QPA_PLATFORM=offscreen is set by
    // the test harness, so no real display is needed.
    QApplication app(argc, argv);

    try
    {
        auto tps = TestPDL::defaultParameters();

        IQParameterSetModel modelToBeTested(std::move(tps));

        QAbstractItemModelTester tester(
            &modelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal);

        auto& ps = const_cast<ParameterSet&>(modelToBeTested.getParameterSet());

        // IQ wrapper elements (which connect parameter valueChanged to model
        // dataChanged) are created lazily on first access.  QAbstractItemModel-
        // Tester only recurses into the first child of each parent, so most
        // wrappers are not yet created after the tester is constructed.
        // Walk the full tree now so every wrapper exists and connectSignals()
        // has been called before any QSignalSpy is set up.
        {
            std::function<void(const QModelIndex&)> warmUp =
                [&](const QModelIndex& parent)
            {
                for (int r = 0; r < modelToBeTested.rowCount(parent); ++r)
                {
                    for (int c = 0; c < modelToBeTested.columnCount(parent); ++c)
                        modelToBeTested.flags(modelToBeTested.index(r, c, parent));
                    warmUp(modelToBeTested.index(r, 0, parent));
                }
            };
            warmUp(QModelIndex());
        }

        // ----------------------------------------------------------------
        // Helper lambdas
        // ----------------------------------------------------------------

        // Verify that dataChanged is emitted at least once during action().
        auto checkSignal = [&](const char* tag, auto action)
        {
            insight::CurrentExceptionContext ctx(std::string("checking dataChanged signal for: ") + tag);
            QSignalSpy spy(&modelToBeTested, &QAbstractItemModel::dataChanged);
            action();
            if (spy.isEmpty())
                throw std::runtime_error(
                    std::string("dataChanged not emitted for: ") + tag);
        };

        // Verify rowsInserted emitted at least once.
        auto checkRowsInserted = [&](const char* tag, auto action)
        {
            insight::CurrentExceptionContext ctx(std::string("checking rowsInserted for: ") + tag);
            QSignalSpy spy(&modelToBeTested, &QAbstractItemModel::rowsInserted);
            action();
            if (spy.isEmpty())
                throw std::runtime_error(
                    std::string("rowsInserted not emitted for: ") + tag);
        };

        // Verify rowsRemoved emitted at least once.
        auto checkRowsRemoved = [&](const char* tag, auto action)
        {
            insight::CurrentExceptionContext ctx(std::string("checking rowsRemoved for: ") + tag);
            QSignalSpy spy(&modelToBeTested, &QAbstractItemModel::rowsRemoved);
            action();
            if (spy.isEmpty())
                throw std::runtime_error(
                    std::string("rowsRemoved not emitted for: ") + tag);
        };

        // Get the value-column QModelIndex for a slash-separated path.
        auto vi = [&](const std::string& path)
        {
            return modelToBeTested.indexOfPath(path, IQParameterSetModel::valueCol);
        };

        // Call setData with Qt::EditRole and verify it returns true.
        auto checkSetData = [&](const std::string& path, const QVariant& val)
        {
            insight::CurrentExceptionContext ctx("setData(EditRole) for: " + path);
            if (!modelToBeTested.setData(vi(path), val, Qt::EditRole))
                throw std::runtime_error("setData(EditRole) returned false for: " + path);
        };

        // Call setData with Qt::CheckStateRole for boolean parameters.
        auto checkSetDataCS = [&](const std::string& path, Qt::CheckState cs)
        {
            insight::CurrentExceptionContext ctx("setData(CheckStateRole) for: " + path);
            if (!modelToBeTested.setData(
                    vi(path), QVariant::fromValue(cs), Qt::CheckStateRole))
                throw std::runtime_error(
                    "setData(CheckStateRole) returned false for: " + path);
        };

        // Throw with path + expected + actual when a post-setData value check fails.
        auto checkVal = [](bool ok,
                           const std::string& path,
                           const std::string& expected,
                           const std::string& actual)
        {
            insight::CurrentExceptionContext ctx("checkVal at: " + path);
            if (!ok)
                throw std::runtime_error(
                    "setData value mismatch at \"" + path
                    + "\": expected " + expected + ", got " + actual);
        };

        // Format a 3-element arma column vector as "(x y z)".
        auto vecStr = [](const arma::mat& v)
        {
            return "(" + std::to_string(v(0)) + " "
                       + std::to_string(v(1)) + " "
                       + std::to_string(v(2)) + ")";
        };

        // ================================================================
        // DoubleParameter
        // ================================================================
        checkSignal("DoubleParameter set()",
            [&]{ ps.get<DoubleParameter>("ap/0").set(99.0); });

        checkSetData("ap/0", QVariant("123.0"));
        { const auto actual = ps.get<DoubleParameter>("ap/0")();
          checkVal(std::abs(actual - 123.0) <= 1e-9, "ap/0", "123.0", std::to_string(actual)); }

        // ================================================================
        // IntParameter
        // ================================================================
        checkSignal("IntParameter set()",
            [&]{ ps.get<IntParameter>("run/initialization/preRuns/resolutions/0/np_coarse").set(8); });

        checkSetData("run/initialization/preRuns/resolutions/0/np_coarse", QVariant("16"));
        { const auto actual = ps.get<IntParameter>("run/initialization/preRuns/resolutions/0/np_coarse")();
          checkVal(actual == 16, "run/initialization/preRuns/resolutions/0/np_coarse", "16", std::to_string(actual)); }

        // ================================================================
        // BoolParameter  (setData via Qt::CheckStateRole)
        // ================================================================
        checkSignal("BoolParameter set()",
            [&]{ ps.get<BoolParameter>("myBool").set(false); });

        checkSetDataCS("myBool", Qt::Checked);
        { const bool actual = ps.get<BoolParameter>("myBool")();
          checkVal(actual, "myBool", "true (Checked)", actual ? "true" : "false"); }

        // ================================================================
        // StringParameter
        // ================================================================
        checkSignal("StringParameter set()",
            [&]{ ps.get<StringParameter>("myString").set("modified"); });

        checkSetData("myString", QVariant(QString("via_setdata")));
        { const auto actual = ps.get<StringParameter>("myString")();
          checkVal(actual == "via_setdata", "myString", "\"via_setdata\"", "\"" + actual + "\""); }

        // ================================================================
        // DateParameter  (boost simple-string format "YYYY-Mon-DD")
        // ================================================================
        checkSignal("DateParameter set()",
            [&]{ ps.get<DateParameter>("myDate").set(boost::gregorian::date(2025, 6, 15)); });

        checkSetData("myDate", QVariant("2025-Jul-04"));
        { const auto actual = ps.get<DateParameter>("myDate")();
          checkVal(actual == boost::gregorian::date(2025, 7, 4),
                   "myDate", "2025-Jul-04", boost::gregorian::to_simple_string(actual)); }

        // ================================================================
        // DateTimeParameter  (boost format "YYYY-Mon-DD HH:MM:SS")
        // ================================================================
        checkSignal("DateTimeParameter set()",
            [&]{ ps.get<DateTimeParameter>("myDateTime").set(
                     boost::posix_time::ptime(
                         boost::gregorian::date(2025, 6, 15),
                         boost::posix_time::time_duration(14, 30, 0))); });

        checkSetData("myDateTime", QVariant("2025-Aug-15 09:30:00"));
        { const auto actual = ps.get<DateTimeParameter>("myDateTime")();
          const auto expected = boost::posix_time::ptime(
              boost::gregorian::date(2025, 8, 15),
              boost::posix_time::time_duration(9, 30, 0));
          checkVal(actual == expected, "myDateTime",
                   boost::posix_time::to_simple_string(expected),
                   boost::posix_time::to_simple_string(actual)); }

        // ================================================================
        // VectorParameter  (space-separated components)
        // ================================================================
        checkSignal("VectorParameter set()",
            [&]{ ps.get<VectorParameter>("myVec").set(arma::mat({2.0, 3.0, 4.0}).t()); });

        checkSetData("myVec", QVariant("5.0 6.0 7.0"));
        { const auto actual = ps.get<VectorParameter>("myVec")();
          checkVal(std::abs(actual(0)-5.0)<=1e-9 && std::abs(actual(1)-6.0)<=1e-9 && std::abs(actual(2)-7.0)<=1e-9,
                   "myVec", "(5 6 7)", vecStr(actual)); }

        // ================================================================
        // MatrixParameter  (no setData support — signal only)
        // ================================================================
        checkSignal("MatrixParameter set()",
            [&]{ ps.get<MatrixParameter>("matrix").set(arma::mat{{5.0, 6.0}, {7.0, 8.0}}); });

        // ================================================================
        // scalarLengthParameter / dimensionedScalar  (signal only)
        // ================================================================
        checkSignal("scalarLengthParameter setInDefaultUnit()",
            [&]{ ps.get<scalarLengthParameter>("L").setInDefaultUnit(2.0); });

        // ================================================================
        // PathParameter  (signal only)
        // ================================================================
        checkSignal("PathParameter setFilePath()",
            [&]{ ps.get<PathParameter>("mapFrom").setFilePath("changed.stl"); });

        // ================================================================
        // DirectoryParameter  (signal only)
        // ================================================================
        checkSignal("DirectoryParameter setFilePath()",
            [&]{ ps.get<DirectoryParameter>("operation/DICOMdata").setFilePath("/tmp/changed_dir"); });

        // ================================================================
        // SpatialTransformationParameter  (signal only)
        // ================================================================
        checkSignal("SpatialTransformationParameter set()",
            [&]{ ps.get<SpatialTransformationParameter>("trsf").set(SpatialTransformation(2.0)); });

        // ================================================================
        // SelectionParameter  (setData via integer index)
        // sel options: (one=0  two=1  three=2)
        // ================================================================
        checkSignal("SelectionParameter setSelection()",
            [&]{ ps.get<SelectionParameter>("sel").setSelection("two"); });

        checkSetData("sel", QVariant(2)); // index 2 → "three"
        { const auto actual = ps.get<SelectionParameter>("sel").selection();
          checkVal(actual == "three", "sel", "three", actual); }

        // ================================================================
        // SelectableSubsetParameter  (structural: rowsRemoved + rowsInserted)
        // ================================================================
        // Switch from unsteady → steady: removes unsteady's child, inserts steady's child
        checkRowsRemoved("SelectableSubsetParameter steady",
            [&]{ ps.get<SelectableSubsetParameter>("run/regime").setSelection("steady"); });
        // Switch back steady → unsteady
        checkRowsInserted("SelectableSubsetParameter unsteady",
            [&]{ ps.get<SelectableSubsetParameter>("run/regime").setSelection("unsteady"); });

        // ================================================================
        // DoubleRangeParameter  (signal only)
        // ================================================================
        checkSignal("DoubleRangeParameter resetValues()",
            [&]{ ps.get<DoubleRangeParameter>("dr").resetValues({10.0, 20.0, 30.0, 40.0}); });

        // ================================================================
        // ArrayParameter structural (rowsInserted + rowsRemoved)
        // ================================================================
        {
            auto apIdx = modelToBeTested.indexOfPath("ap", 0);
            checkRowsInserted("ArrayParameter append",
                [&]{ modelToBeTested.appendArrayElement(apIdx, DoubleParameter(77.0, "")); });
            checkRowsRemoved("ArrayParameter remove",
                [&]{ modelToBeTested.removeArrayElement(
                         modelToBeTested.index(modelToBeTested.rowCount(apIdx)-1, 0, apIdx)); });
        }

        // ================================================================
        // LabeledArrayParameter structural (rowsRemoved)
        // ================================================================
        {
            auto wallsIdx = modelToBeTested.indexOfPath("geometry/walls", 0);
            if (modelToBeTested.rowCount(wallsIdx) > 0)
            {
                checkRowsRemoved("LabeledArrayParameter remove",
                    [&]{ modelToBeTested.removeLabeledArrayElement(
                             modelToBeTested.index(0, 0, wallsIdx)); });
            }
        }

        // ================================================================
        // Types inside 'set' container (allTypes.*)
        // ================================================================
        checkSignal("set/double set()",
            [&]{ ps.get<DoubleParameter>("allTypes/atsDbl").set(11.0); });
        checkSetData("allTypes/atsDbl", QVariant("22.0"));
        { const auto actual = ps.get<DoubleParameter>("allTypes/atsDbl")();
          checkVal(std::abs(actual - 22.0) <= 1e-9, "allTypes/atsDbl", "22.0", std::to_string(actual)); }

        checkSignal("set/int set()",
            [&]{ ps.get<IntParameter>("allTypes/atsInt").set(9); });
        checkSetData("allTypes/atsInt", QVariant("18"));
        { const auto actual = ps.get<IntParameter>("allTypes/atsInt")();
          checkVal(actual == 18, "allTypes/atsInt", "18", std::to_string(actual)); }

        checkSignal("set/bool set()",
            [&]{ ps.get<BoolParameter>("allTypes/atsBool").set(false); });
        checkSetDataCS("allTypes/atsBool", Qt::Checked);
        { const bool actual = ps.get<BoolParameter>("allTypes/atsBool")();
          checkVal(actual, "allTypes/atsBool", "true (Checked)", actual ? "true" : "false"); }

        checkSignal("set/string set()",
            [&]{ ps.get<StringParameter>("allTypes/atsStr").set("in_set"); });
        checkSetData("allTypes/atsStr", QVariant(QString("set_via_setdata")));
        { const auto actual = ps.get<StringParameter>("allTypes/atsStr")();
          checkVal(actual == "set_via_setdata", "allTypes/atsStr",
                   "\"set_via_setdata\"", "\"" + actual + "\""); }

        checkSignal("set/date set()",
            [&]{ ps.get<DateParameter>("allTypes/atsDate").set(boost::gregorian::date(2025, 9, 1)); });
        checkSetData("allTypes/atsDate", QVariant("2025-Oct-01"));
        { const auto actual = ps.get<DateParameter>("allTypes/atsDate")();
          checkVal(actual == boost::gregorian::date(2025, 10, 1),
                   "allTypes/atsDate", "2025-Oct-01", boost::gregorian::to_simple_string(actual)); }

        checkSignal("set/datetime set()",
            [&]{ ps.get<DateTimeParameter>("allTypes/atsDt").set(
                     boost::posix_time::ptime(boost::gregorian::date(2025, 9, 1),
                                              boost::posix_time::time_duration(8, 0, 0))); });
        checkSetData("allTypes/atsDt", QVariant("2025-Oct-01 10:00:00"));
        { const auto actual = ps.get<DateTimeParameter>("allTypes/atsDt")();
          const auto expected = boost::posix_time::ptime(boost::gregorian::date(2025, 10, 1),
                                                          boost::posix_time::time_duration(10, 0, 0));
          checkVal(actual == expected, "allTypes/atsDt",
                   boost::posix_time::to_simple_string(expected),
                   boost::posix_time::to_simple_string(actual)); }

        checkSignal("set/vector set()",
            [&]{ ps.get<VectorParameter>("allTypes/atsVec").set(arma::mat({0.0, 0.0, 1.0}).t()); });
        checkSetData("allTypes/atsVec", QVariant("3.0 4.0 5.0"));
        { const auto actual = ps.get<VectorParameter>("allTypes/atsVec")();
          checkVal(std::abs(actual(0)-3.0)<=1e-9 && std::abs(actual(1)-4.0)<=1e-9 && std::abs(actual(2)-5.0)<=1e-9,
                   "allTypes/atsVec", "(3 4 5)", vecStr(actual)); }

        checkSignal("set/matrix set()",
            [&]{ ps.get<MatrixParameter>("allTypes/atsMat").set(arma::mat{{3.0, 0.0}, {0.0, 3.0}}); });

        checkSignal("set/dimensionedScalar setInDefaultUnit()",
            [&]{ ps.get<scalarLengthParameter>("allTypes/atsDimSc").setInDefaultUnit(5.0); });

        // atsSel options: (atsCat=0  atsDog=1  atsBird=2)
        checkSignal("set/selection setSelection()",
            [&]{ ps.get<SelectionParameter>("allTypes/atsSel").setSelection("atsDog"); });
        checkSetData("allTypes/atsSel", QVariant(2)); // index 2 → atsBird
        { const auto actual = ps.get<SelectionParameter>("allTypes/atsSel").selection();
          checkVal(actual == "atsBird", "allTypes/atsSel", "atsBird", actual); }

        checkSignal("set/doubleRange resetValues()",
            [&]{ ps.get<DoubleRangeParameter>("allTypes/atsDr").resetValues({15.0, 25.0, 35.0}); });

        checkSignal("set/path setFilePath()",
            [&]{ ps.get<PathParameter>("allTypes/atsPath").setFilePath("set_path.stl"); });

        checkSignal("set/directory setFilePath()",
            [&]{ ps.get<DirectoryParameter>("allTypes/atsDir").setFilePath("/tmp/set_dir"); });

        checkSignal("set/trsf set()",
            [&]{ ps.get<SpatialTransformationParameter>("allTypes/atsTrsf").set(SpatialTransformation(3.0)); });

        // ================================================================
        // Types inside selectablesubset (allTypes/atsSS.*)
        // ================================================================
        // atssChoiceA → atssChoiceB: many rows removed, 1 inserted
        checkRowsRemoved("atsSS → atssChoiceB",
            [&]{ ps.get<SelectableSubsetParameter>("allTypes/atsSS").setSelection("atssChoiceB"); });
        // atssChoiceB → atssChoiceA: 1 row removed, many inserted
        checkRowsInserted("atsSS → atssChoiceA",
            [&]{ ps.get<SelectableSubsetParameter>("allTypes/atsSS").setSelection("atssChoiceA"); });

        checkSignal("ss/double set()",
            [&]{ ps.get<DoubleParameter>("allTypes/atsSS/atssaDbl").set(88.0); });
        checkSetData("allTypes/atsSS/atssaDbl", QVariant("44.0"));
        { const auto actual = ps.get<DoubleParameter>("allTypes/atsSS/atssaDbl")();
          checkVal(std::abs(actual - 44.0) <= 1e-9, "allTypes/atsSS/atssaDbl", "44.0", std::to_string(actual)); }

        checkSignal("ss/int set()",
            [&]{ ps.get<IntParameter>("allTypes/atsSS/atssaInt").set(5); });
        checkSetData("allTypes/atsSS/atssaInt", QVariant("10"));
        { const auto actual = ps.get<IntParameter>("allTypes/atsSS/atssaInt")();
          checkVal(actual == 10, "allTypes/atsSS/atssaInt", "10", std::to_string(actual)); }

        checkSignal("ss/bool set()",
            [&]{ ps.get<BoolParameter>("allTypes/atsSS/atssaBool").set(false); });
        checkSetDataCS("allTypes/atsSS/atssaBool", Qt::Unchecked);
        { const bool actual = ps.get<BoolParameter>("allTypes/atsSS/atssaBool")();
          checkVal(!actual, "allTypes/atsSS/atssaBool", "false (Unchecked)", actual ? "true" : "false"); }

        checkSignal("ss/string set()",
            [&]{ ps.get<StringParameter>("allTypes/atsSS/atssaStr").set("ss_val"); });
        checkSetData("allTypes/atsSS/atssaStr", QVariant(QString("ss_setdata")));
        { const auto actual = ps.get<StringParameter>("allTypes/atsSS/atssaStr")();
          checkVal(actual == "ss_setdata", "allTypes/atsSS/atssaStr",
                   "\"ss_setdata\"", "\"" + actual + "\""); }

        checkSignal("ss/date set()",
            [&]{ ps.get<DateParameter>("allTypes/atsSS/atssaDate").set(boost::gregorian::date(2025, 3, 1)); });
        checkSetData("allTypes/atsSS/atssaDate", QVariant("2025-Apr-01"));
        { const auto actual = ps.get<DateParameter>("allTypes/atsSS/atssaDate")();
          checkVal(actual == boost::gregorian::date(2025, 4, 1),
                   "allTypes/atsSS/atssaDate", "2025-Apr-01", boost::gregorian::to_simple_string(actual)); }

        checkSignal("ss/datetime set()",
            [&]{ ps.get<DateTimeParameter>("allTypes/atsSS/atssaDt").set(
                     boost::posix_time::ptime(boost::gregorian::date(2025, 3, 1),
                                              boost::posix_time::time_duration(12, 0, 0))); });
        checkSetData("allTypes/atsSS/atssaDt", QVariant("2025-May-01 06:00:00"));
        { const auto actual = ps.get<DateTimeParameter>("allTypes/atsSS/atssaDt")();
          const auto expected = boost::posix_time::ptime(boost::gregorian::date(2025, 5, 1),
                                                          boost::posix_time::time_duration(6, 0, 0));
          checkVal(actual == expected, "allTypes/atsSS/atssaDt",
                   boost::posix_time::to_simple_string(expected),
                   boost::posix_time::to_simple_string(actual)); }

        checkSignal("ss/vector set()",
            [&]{ ps.get<VectorParameter>("allTypes/atsSS/atssaVec").set(arma::mat({1.0, 0.0, 0.0}).t()); });
        checkSetData("allTypes/atsSS/atssaVec", QVariant("6.0 7.0 8.0"));
        { const auto actual = ps.get<VectorParameter>("allTypes/atsSS/atssaVec")();
          checkVal(std::abs(actual(0)-6.0)<=1e-9 && std::abs(actual(1)-7.0)<=1e-9 && std::abs(actual(2)-8.0)<=1e-9,
                   "allTypes/atsSS/atssaVec", "(6 7 8)", vecStr(actual)); }

        checkSignal("ss/matrix set()",
            [&]{ ps.get<MatrixParameter>("allTypes/atsSS/atssaMat").set(arma::mat{{2.0, 0.0}, {0.0, 2.0}}); });

        // atssaSel options: (atssaUp=0  atssaDown=1)
        checkSignal("ss/selection setSelection()",
            [&]{ ps.get<SelectionParameter>("allTypes/atsSS/atssaSel").setSelection("atssaDown"); });
        checkSetData("allTypes/atsSS/atssaSel", QVariant(0)); // index 0 → atssaUp
        { const auto actual = ps.get<SelectionParameter>("allTypes/atsSS/atssaSel").selection();
          checkVal(actual == "atssaUp", "allTypes/atsSS/atssaSel", "atssaUp", actual); }

        checkSignal("ss/doubleRange resetValues()",
            [&]{ ps.get<DoubleRangeParameter>("allTypes/atsSS/atssaDr").resetValues({3.0, 6.0}); });

        checkSignal("ss/path setFilePath()",
            [&]{ ps.get<PathParameter>("allTypes/atsSS/atssaPath").setFilePath("ss_path.stl"); });

        checkSignal("ss/directory setFilePath()",
            [&]{ ps.get<DirectoryParameter>("allTypes/atsSS/atssaDir").setFilePath("/tmp/ss_dir"); });

        checkSignal("ss/trsf set()",
            [&]{ ps.get<SpatialTransformationParameter>("allTypes/atsSS/atssaTrsf").set(SpatialTransformation(4.0)); });

        // Array inside selectablesubset — structural
        {
            auto ssArrIdx = modelToBeTested.indexOfPath("allTypes/atsSS/atssaArr", 0);
            checkRowsInserted("atssaArr append",
                [&]{ modelToBeTested.appendArrayElement(ssArrIdx, DoubleParameter(33.0, "")); });
            checkRowsRemoved("atssaArr remove",
                [&]{ modelToBeTested.removeArrayElement(
                         modelToBeTested.index(
                             modelToBeTested.rowCount(ssArrIdx)-1, 0, ssArrIdx)); });
        }

        // ================================================================
        // Types inside array element sets (allTypes/atsArr/0/*)
        // ================================================================
        checkSignal("arr/double set()",
            [&]{ ps.get<DoubleParameter>("allTypes/atsArr/0/atsarrDbl").set(42.0); });
        checkSetData("allTypes/atsArr/0/atsarrDbl", QVariant("84.0"));
        { const auto actual = ps.get<DoubleParameter>("allTypes/atsArr/0/atsarrDbl")();
          checkVal(std::abs(actual - 84.0) <= 1e-9, "allTypes/atsArr/0/atsarrDbl", "84.0", std::to_string(actual)); }

        checkSignal("arr/int set()",
            [&]{ ps.get<IntParameter>("allTypes/atsArr/0/atsarrInt").set(99); });
        checkSetData("allTypes/atsArr/0/atsarrInt", QVariant("50"));
        { const auto actual = ps.get<IntParameter>("allTypes/atsArr/0/atsarrInt")();
          checkVal(actual == 50, "allTypes/atsArr/0/atsarrInt", "50", std::to_string(actual)); }

        checkSignal("arr/bool set()",
            [&]{ ps.get<BoolParameter>("allTypes/atsArr/0/atsarrBool").set(true); });
        checkSetDataCS("allTypes/atsArr/0/atsarrBool", Qt::Unchecked);
        { const bool actual = ps.get<BoolParameter>("allTypes/atsArr/0/atsarrBool")();
          checkVal(!actual, "allTypes/atsArr/0/atsarrBool", "false (Unchecked)", actual ? "true" : "false"); }

        checkSignal("arr/string set()",
            [&]{ ps.get<StringParameter>("allTypes/atsArr/0/atsarrStr").set("arr_val"); });
        checkSetData("allTypes/atsArr/0/atsarrStr", QVariant(QString("arr_setdata")));
        { const auto actual = ps.get<StringParameter>("allTypes/atsArr/0/atsarrStr")();
          checkVal(actual == "arr_setdata", "allTypes/atsArr/0/atsarrStr",
                   "\"arr_setdata\"", "\"" + actual + "\""); }

        checkSignal("arr/date set()",
            [&]{ ps.get<DateParameter>("allTypes/atsArr/0/atsarrDate").set(boost::gregorian::date(2025, 7, 4)); });
        checkSetData("allTypes/atsArr/0/atsarrDate", QVariant("2025-Nov-01"));
        { const auto actual = ps.get<DateParameter>("allTypes/atsArr/0/atsarrDate")();
          checkVal(actual == boost::gregorian::date(2025, 11, 1),
                   "allTypes/atsArr/0/atsarrDate", "2025-Nov-01", boost::gregorian::to_simple_string(actual)); }

        checkSignal("arr/datetime set()",
            [&]{ ps.get<DateTimeParameter>("allTypes/atsArr/0/atsarrDt").set(
                     boost::posix_time::ptime(boost::gregorian::date(2025, 7, 4),
                                              boost::posix_time::time_duration(10, 0, 0))); });
        checkSetData("allTypes/atsArr/0/atsarrDt", QVariant("2025-Dec-01 08:00:00"));
        { const auto actual = ps.get<DateTimeParameter>("allTypes/atsArr/0/atsarrDt")();
          const auto expected = boost::posix_time::ptime(boost::gregorian::date(2025, 12, 1),
                                                          boost::posix_time::time_duration(8, 0, 0));
          checkVal(actual == expected, "allTypes/atsArr/0/atsarrDt",
                   boost::posix_time::to_simple_string(expected),
                   boost::posix_time::to_simple_string(actual)); }

        checkSignal("arr/vector set()",
            [&]{ ps.get<VectorParameter>("allTypes/atsArr/0/atsarrVec").set(arma::mat({1.0, 0.0, 0.0}).t()); });
        checkSetData("allTypes/atsArr/0/atsarrVec", QVariant("9.0 8.0 7.0"));
        { const auto actual = ps.get<VectorParameter>("allTypes/atsArr/0/atsarrVec")();
          checkVal(std::abs(actual(0)-9.0)<=1e-9 && std::abs(actual(1)-8.0)<=1e-9 && std::abs(actual(2)-7.0)<=1e-9,
                   "allTypes/atsArr/0/atsarrVec", "(9 8 7)", vecStr(actual)); }

        checkSignal("arr/matrix set()",
            [&]{ ps.get<MatrixParameter>("allTypes/atsArr/0/atsarrMat").set(arma::mat{{4.0, 0.0}, {0.0, 4.0}}); });

        // atsarrSel options: (atsarrP=0  atsarrQ=1  atsarrR=2)
        checkSignal("arr/selection setSelection()",
            [&]{ ps.get<SelectionParameter>("allTypes/atsArr/0/atsarrSel").setSelection("atsarrQ"); });
        checkSetData("allTypes/atsArr/0/atsarrSel", QVariant(2)); // index 2 → atsarrR
        { const auto actual = ps.get<SelectionParameter>("allTypes/atsArr/0/atsarrSel").selection();
          checkVal(actual == "atsarrR", "allTypes/atsArr/0/atsarrSel", "atsarrR", actual); }

        checkSignal("arr/doubleRange resetValues()",
            [&]{ ps.get<DoubleRangeParameter>("allTypes/atsArr/0/atsarrDr").resetValues({5.0, 10.0, 15.0}); });

        checkSignal("arr/path setFilePath()",
            [&]{ ps.get<PathParameter>("allTypes/atsArr/0/atsarrPath").setFilePath("arr_path.stl"); });

        checkSignal("arr/directory setFilePath()",
            [&]{ ps.get<DirectoryParameter>("allTypes/atsArr/0/atsarrDir").setFilePath("/tmp/arr_dir"); });

        checkSignal("arr/trsf set()",
            [&]{ ps.get<SpatialTransformationParameter>("allTypes/atsArr/0/atsarrTrsf").set(SpatialTransformation(5.0)); });

        // Structural: append + remove an element of allTypes/atsArr
        {
            auto atsArrIdx = modelToBeTested.indexOfPath("allTypes/atsArr", 0);
            checkRowsInserted("atsArr append",
                [&]{ modelToBeTested.appendArrayElement(
                         atsArrIdx,
                         ps.get<ArrayParameter>("allTypes/atsArr").defaultValue()); });
            checkRowsRemoved("atsArr remove",
                [&]{ modelToBeTested.removeArrayElement(
                         modelToBeTested.index(
                             modelToBeTested.rowCount(atsArrIdx)-1, 0, atsArrIdx)); });
        }

        // ================================================================
        // Types inside labeledarray element sets (allTypes/atsLabArr/<key>/*)
        // ================================================================
        {
            auto& lap = ps.get<LabeledArrayParameter>("allTypes/atsLabArr");
            const auto firstKey = *lap.keys().begin();
            const auto p = "allTypes/atsLabArr/" + firstKey + "/";

            checkSignal("la/double set()",
                [&]{ ps.get<DoubleParameter>(p + "atslaDbl").set(55.0); });
            checkSetData(p + "atslaDbl", QVariant("110.0"));
            { const auto actual = ps.get<DoubleParameter>(p + "atslaDbl")();
              checkVal(std::abs(actual - 110.0) <= 1e-9, p + "atslaDbl", "110.0", std::to_string(actual)); }

            checkSignal("la/int set()",
                [&]{ ps.get<IntParameter>(p + "atslaInt").set(77); });
            checkSetData(p + "atslaInt", QVariant("33"));
            { const auto actual = ps.get<IntParameter>(p + "atslaInt")();
              checkVal(actual == 33, p + "atslaInt", "33", std::to_string(actual)); }

            checkSignal("la/bool set()",
                [&]{ ps.get<BoolParameter>(p + "atslaBool").set(true); });
            checkSetDataCS(p + "atslaBool", Qt::Unchecked);
            { const bool actual = ps.get<BoolParameter>(p + "atslaBool")();
              checkVal(!actual, p + "atslaBool", "false (Unchecked)", actual ? "true" : "false"); }

            checkSignal("la/string set()",
                [&]{ ps.get<StringParameter>(p + "atslaStr").set("la_val"); });
            checkSetData(p + "atslaStr", QVariant(QString("la_setdata")));
            { const auto actual = ps.get<StringParameter>(p + "atslaStr")();
              checkVal(actual == "la_setdata", p + "atslaStr",
                       "\"la_setdata\"", "\"" + actual + "\""); }

            checkSignal("la/date set()",
                [&]{ ps.get<DateParameter>(p + "atslaDate").set(boost::gregorian::date(2025, 8, 15)); });
            checkSetData(p + "atslaDate", QVariant("2025-Sep-01"));
            { const auto actual = ps.get<DateParameter>(p + "atslaDate")();
              checkVal(actual == boost::gregorian::date(2025, 9, 1),
                       p + "atslaDate", "2025-Sep-01", boost::gregorian::to_simple_string(actual)); }

            checkSignal("la/datetime set()",
                [&]{ ps.get<DateTimeParameter>(p + "atslaDt").set(
                         boost::posix_time::ptime(boost::gregorian::date(2025, 8, 15),
                                                  boost::posix_time::time_duration(9, 0, 0))); });
            checkSetData(p + "atslaDt", QVariant("2025-Oct-15 07:00:00"));
            { const auto actual = ps.get<DateTimeParameter>(p + "atslaDt")();
              const auto expected = boost::posix_time::ptime(boost::gregorian::date(2025, 10, 15),
                                                              boost::posix_time::time_duration(7, 0, 0));
              checkVal(actual == expected, p + "atslaDt",
                       boost::posix_time::to_simple_string(expected),
                       boost::posix_time::to_simple_string(actual)); }

            checkSignal("la/vector set()",
                [&]{ ps.get<VectorParameter>(p + "atslaVec").set(arma::mat({0.0, 1.0, 0.0}).t()); });
            checkSetData(p + "atslaVec", QVariant("1.1 2.2 3.3"));
            { const auto actual = ps.get<VectorParameter>(p + "atslaVec")();
              checkVal(std::abs(actual(0)-1.1)<=1e-9 && std::abs(actual(1)-2.2)<=1e-9 && std::abs(actual(2)-3.3)<=1e-9,
                       p + "atslaVec", "(1.1 2.2 3.3)", vecStr(actual)); }

            checkSignal("la/matrix set()",
                [&]{ ps.get<MatrixParameter>(p + "atslaMat").set(arma::mat{{5.0, 0.0}, {0.0, 5.0}}); });

            // atslaSel options: (atslaU=0  atslaV=1  atslaW=2)
            checkSignal("la/selection setSelection()",
                [&]{ ps.get<SelectionParameter>(p + "atslaSel").setSelection("atslaV"); });
            checkSetData(p + "atslaSel", QVariant(2)); // index 2 → atslaW
            { const auto actual = ps.get<SelectionParameter>(p + "atslaSel").selection();
              checkVal(actual == "atslaW", p + "atslaSel", "atslaW", actual); }

            checkSignal("la/doubleRange resetValues()",
                [&]{ ps.get<DoubleRangeParameter>(p + "atslaDr").resetValues({100.0, 150.0}); });

            checkSignal("la/path setFilePath()",
                [&]{ ps.get<PathParameter>(p + "atslaPath").setFilePath("la_path.stl"); });

            checkSignal("la/directory setFilePath()",
                [&]{ ps.get<DirectoryParameter>(p + "atslaDir").setFilePath("/tmp/la_dir"); });

            checkSignal("la/trsf set()",
                [&]{ ps.get<SpatialTransformationParameter>(p + "atslaTrsf").set(SpatialTransformation(6.0)); });
        }

        // ================================================================
        // Types inside includedset (subps/*)
        // ================================================================
        checkSignal("inc/int set()",
            [&]{ ps.get<IntParameter>("subps/subInt").set(7); });
        checkSetData("subps/subInt", QVariant("14"));
        { const auto actual = ps.get<IntParameter>("subps/subInt")();
          checkVal(actual == 14, "subps/subInt", "14", std::to_string(actual)); }

        checkSignal("inc/bool set()",
            [&]{ ps.get<BoolParameter>("subps/subBool").set(true); });
        checkSetDataCS("subps/subBool", Qt::Unchecked);
        { const bool actual = ps.get<BoolParameter>("subps/subBool")();
          checkVal(!actual, "subps/subBool", "false (Unchecked)", actual ? "true" : "false"); }

        checkSignal("inc/string set()",
            [&]{ ps.get<StringParameter>("subps/subString").set("sub_modified"); });
        checkSetData("subps/subString", QVariant(QString("sub_setdata")));
        { const auto actual = ps.get<StringParameter>("subps/subString")();
          checkVal(actual == "sub_setdata", "subps/subString",
                   "\"sub_setdata\"", "\"" + actual + "\""); }

        checkSignal("inc/date set()",
            [&]{ ps.get<DateParameter>("subps/subDate").set(boost::gregorian::date(2025, 3, 15)); });
        checkSetData("subps/subDate", QVariant("2025-Apr-15"));
        { const auto actual = ps.get<DateParameter>("subps/subDate")();
          checkVal(actual == boost::gregorian::date(2025, 4, 15),
                   "subps/subDate", "2025-Apr-15", boost::gregorian::to_simple_string(actual)); }

        checkSignal("inc/datetime set()",
            [&]{ ps.get<DateTimeParameter>("subps/subDt").set(
                     boost::posix_time::ptime(boost::gregorian::date(2025, 3, 15),
                                              boost::posix_time::time_duration(11, 0, 0))); });
        checkSetData("subps/subDt", QVariant("2025-May-15 13:00:00"));
        { const auto actual = ps.get<DateTimeParameter>("subps/subDt")();
          const auto expected = boost::posix_time::ptime(boost::gregorian::date(2025, 5, 15),
                                                          boost::posix_time::time_duration(13, 0, 0));
          checkVal(actual == expected, "subps/subDt",
                   boost::posix_time::to_simple_string(expected),
                   boost::posix_time::to_simple_string(actual)); }

        checkSignal("inc/vector set()",
            [&]{ ps.get<VectorParameter>("subps/subVec").set(arma::mat({0.0, 0.0, 1.0}).t()); });
        checkSetData("subps/subVec", QVariant("4.0 5.0 6.0"));
        { const auto actual = ps.get<VectorParameter>("subps/subVec")();
          checkVal(std::abs(actual(0)-4.0)<=1e-9 && std::abs(actual(1)-5.0)<=1e-9 && std::abs(actual(2)-6.0)<=1e-9,
                   "subps/subVec", "(4 5 6)", vecStr(actual)); }

        checkSignal("inc/matrix set()",
            [&]{ ps.get<MatrixParameter>("subps/subMat").set(arma::mat{{4.0, 0.0}, {0.0, 4.0}}); });

        // subSel options: (subSelA=0  subSelB=1  subSelC=2)
        checkSignal("inc/selection setSelection()",
            [&]{ ps.get<SelectionParameter>("subps/subSel").setSelection("subSelB"); });
        checkSetData("subps/subSel", QVariant(2)); // index 2 → subSelC
        { const auto actual = ps.get<SelectionParameter>("subps/subSel").selection();
          checkVal(actual == "subSelC", "subps/subSel", "subSelC", actual); }

        checkSignal("inc/doubleRange resetValues()",
            [&]{ ps.get<DoubleRangeParameter>("subps/subDr").resetValues({5.0, 10.0}); });

        checkSignal("inc/path setFilePath()",
            [&]{ ps.get<PathParameter>("subps/subPath").setFilePath("sub_path.stl"); });

        checkSignal("inc/directory setFilePath()",
            [&]{ ps.get<DirectoryParameter>("subps/subDir").setFilePath("/tmp/sub_dir"); });

        checkSignal("inc/trsf set()",
            [&]{ ps.get<SpatialTransformationParameter>("subps/subTrsf").set(SpatialTransformation(3.0)); });

        // SelectableSubset inside includedset
        checkRowsRemoved("subps/subss → subssChoiceY",
            [&]{ ps.get<SelectableSubsetParameter>("subps/subss").setSelection("subssChoiceY"); });
        checkRowsInserted("subps/subss → subssChoiceX",
            [&]{ ps.get<SelectableSubsetParameter>("subps/subss").setSelection("subssChoiceX"); });

        // Types inside active alternative of selectablesubset in includedset
        checkSignal("inc/ss/double set()",
            [&]{ ps.get<DoubleParameter>("subps/subss/subssxDbl").set(22.0); });
        checkSetData("subps/subss/subssxDbl", QVariant("44.0"));
        { const auto actual = ps.get<DoubleParameter>("subps/subss/subssxDbl")();
          checkVal(std::abs(actual - 44.0) <= 1e-9, "subps/subss/subssxDbl", "44.0", std::to_string(actual)); }

        checkSignal("inc/ss/bool set()",
            [&]{ ps.get<BoolParameter>("subps/subss/subssxBool").set(false); });
        checkSetDataCS("subps/subss/subssxBool", Qt::Checked);
        { const bool actual = ps.get<BoolParameter>("subps/subss/subssxBool")();
          checkVal(actual, "subps/subss/subssxBool", "true (Checked)", actual ? "true" : "false"); }

        // subssxSel options: (subssxP=0  subssxQ=1)
        checkSignal("inc/ss/selection setSelection()",
            [&]{ ps.get<SelectionParameter>("subps/subss/subssxSel").setSelection("subssxQ"); });
        checkSetData("subps/subss/subssxSel", QVariant(0)); // index 0 → subssxP
        { const auto actual = ps.get<SelectionParameter>("subps/subss/subssxSel").selection();
          checkVal(actual == "subssxP", "subps/subss/subssxSel", "subssxP", actual); }

        // Array in includedset — structural
        {
            auto subArrIdx = modelToBeTested.indexOfPath("subps/subArr", 0);
            checkRowsInserted("subps/subArr append",
                [&]{ modelToBeTested.appendArrayElement(
                         subArrIdx,
                         ps.get<ArrayParameter>("subps/subArr").defaultValue()); });
            checkRowsRemoved("subps/subArr remove",
                [&]{ modelToBeTested.removeArrayElement(
                         modelToBeTested.index(
                             modelToBeTested.rowCount(subArrIdx)-1, 0, subArrIdx)); });
        }

        // Types inside set inside includedset (subps/subsub)
        checkSignal("inc/subsub/double set()",
            [&]{ ps.get<DoubleParameter>("subps/subsub/subsubDbl").set(33.0); });
        checkSetData("subps/subsub/subsubDbl", QVariant("66.0"));
        { const auto actual = ps.get<DoubleParameter>("subps/subsub/subsubDbl")();
          checkVal(std::abs(actual - 66.0) <= 1e-9, "subps/subsub/subsubDbl", "66.0", std::to_string(actual)); }

        checkSignal("inc/subsub/bool set()",
            [&]{ ps.get<BoolParameter>("subps/subsub/subsubBool").set(false); });
        checkSetDataCS("subps/subsub/subsubBool", Qt::Checked);
        { const bool actual = ps.get<BoolParameter>("subps/subsub/subsubBool")();
          checkVal(actual, "subps/subsub/subsubBool", "true (Checked)", actual ? "true" : "false"); }

        // subsubSel options: (subsubAA=0  subsubBB=1  subsubCC=2)
        checkSignal("inc/subsub/selection setSelection()",
            [&]{ ps.get<SelectionParameter>("subps/subsub/subsubSel").setSelection("subsubBB"); });
        checkSetData("subps/subsub/subsubSel", QVariant(2)); // index 2 → subsubCC
        { const auto actual = ps.get<SelectionParameter>("subps/subsub/subsubSel").selection();
          checkVal(actual == "subsubCC", "subps/subsub/subsubSel", "subsubCC", actual); }

        checkSignal("inc/subsub/doubleRange resetValues()",
            [&]{ ps.get<DoubleRangeParameter>("subps/subsub/subsubDr").resetValues({4.0, 8.0}); });

        // ================================================================
        // resetParameterValues (exercises modelReset / dataChanged on whole tree)
        // ================================================================
        {
            insight::CurrentExceptionContext ex("resetParameterValues test");
            auto tps2 = TestPDL::defaultParameters();
            auto& sk = tps2->get<CADSketchParameter>("sketch");
            sk.setScript(
                "layer standard\n"
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
            modelToBeTested.resetParameterValues(*tps2);
        }

        if (!nogui)
        {
            QDialog dlg;
            auto* l = new QVBoxLayout;
            auto* tv = new QTreeView;
            tv->setItemDelegate(new IQHierarchicalDataGridViewSelectorDelegate);
            l->addWidget(tv);
            tv->setModel(&modelToBeTested);
            tv->setContextMenuPolicy(Qt::CustomContextMenu);
            tv->setAlternatingRowColors(true);
            tv->setDragDropMode(QAbstractItemView::DragDrop);
            tv->setDefaultDropAction(Qt::MoveAction);
            tv->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
            QObject::connect(
                tv, &QTreeView::customContextMenuRequested,
                [tv](const QPoint& p)
                {
                    IQParameterSetModel::contextMenu(tv, tv->indexAt(p), p);
                });
            dlg.setLayout(l);
            QTimer::singleShot(0, [&]{ tv->scrollToBottom(); });
            dlg.exec();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Failed: " << ex.what() << std::endl;
        return -1;
    }
    std::cout << "Passed" << std::endl;
    return 0;
}
