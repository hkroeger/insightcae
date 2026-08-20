#include "base/exception.h"
#include "openfoam/caseelements/evaluation/readpostprocessingfiles.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"

#include <fstream>
#include <iostream>
#include <cmath>

#include <boost/filesystem.hpp>

using namespace insight;
namespace fs = boost::filesystem;


// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static fs::path makeTempDir()
{
    auto p = fs::temp_directory_path() / fs::unique_path();
    fs::create_directories(p);
    return p;
}

static void writeFile(const fs::path& p, const std::string& content)
{
    std::ofstream f(p.string());
    if (!f)
        throw insight::Exception("cannot write " + p.string());
    f << content;
}

static void checkClose(double actual, double expected, const std::string& label)
{
    insight::assertion(
        std::abs(actual - expected) < 1e-12,
        "wrong value for %s: expected %g, got %g",
        label.c_str(), expected, actual);
}


// -----------------------------------------------------------------------
// Within-file tests  (readSingleTabularFile directly)
// -----------------------------------------------------------------------

static void test_noHeader()
{
    auto tmp = makeTempDir();
    writeFile(tmp / "data.dat",
        "0.1 1.0 10.0\n"
        "0.2 2.0 20.0\n"
    );

    std::vector<std::string> names;
    auto result = readSingleTabularFile(tmp / "data.dat", -1, "()", &names);

    insight::assertion(result.count("default") == 1,
        "no-header: expected 'default' group");
    const auto& m = result.at("default");

    insight::assertion(m.n_rows == 2 && m.n_cols == 3,
        "no-header: unexpected matrix shape");
    insight::assertion(names.size() == 3,
        "no-header: expected 3 auto-generated names");
    insight::assertion(names[0] == "column0", "no-header: col0 name");
    insight::assertion(names[1] == "column1", "no-header: col1 name");
    insight::assertion(names[2] == "column2", "no-header: col2 name");
    checkClose(m(0, 0), 0.1,  "no-header: row0 col0");
    checkClose(m(1, 2), 20.0, "no-header: row1 col2");

    fs::remove_all(tmp);
    std::cout << "PASS: test_noHeader" << std::endl;
}

static void test_singleHeader()
{
    auto tmp = makeTempDir();
    writeFile(tmp / "data.dat",
"# Region type :  patch outlet\n"
"# Faces          : 538\n"
"# Area           : 2.349568635e-04\n"
"# Scale factor   : 1.000000000e+00\n"
"# Time           	areaAverage(p)	areaAverage(T)	areaAverage(pTotal)\n"
"0.3866317        	1.151458810e+03	2.819170573e+02	7.359226079e+06\n"
    );

    std::vector<std::string> names;
    auto result = readSingleTabularFile(tmp / "data.dat", -1, "()", &names);
    const auto& m = result.at("default");

    // 4 tab-separated columns, 1 data row; brackets preserved in column names
    insight::assertion(m.n_rows == 1 && m.n_cols == 4,
        "single-header: unexpected matrix shape");
    insight::assertion(names[0] == "Time",                "single-header: col0 name");
    insight::assertion(names[1] == "areaAverage(p)",      "single-header: col1 name");
    insight::assertion(names[2] == "areaAverage(T)",      "single-header: col2 name");
    insight::assertion(names[3] == "areaAverage(pTotal)", "single-header: col3 name");
    checkClose(m(0, 0), 0.3866317,        "single-header: row0 Time");
    checkClose(m(0, 1), 1.151458810e+03,  "single-header: row0 areaAverage(p)");
    checkClose(m(0, 2), 2.819170573e+02,  "single-header: row0 areaAverage(T)");
    checkClose(m(0, 3), 7.359226079e+06,  "single-header: row0 areaAverage(pTotal)");

    fs::remove_all(tmp);
    std::cout << "PASS: test_singleHeader" << std::endl;
}

// Core regression: a column is APPENDED to existing columns mid-file.
// Before second header:  Time  sum(phi)
// After second header:   Time  sum(phi)  sum(phiv)
// sum(phi) must stay in col1; sum(phiv) must be 0.0 for early rows.
static void test_midFileColumnInsertion()
{
    auto tmp = makeTempDir();
    writeFile(tmp / "data.dat",
"# Region type :  patch freesurface\n"
"# Faces          : 15044\n"
"# Area           : 1.015103911e-02\n"
"# Scale factor   : 1.000000000e+00\n"
"# Time           	sum(phi)\n"
"6.61511e-07      	-2.696333733e-01\n"
"1.45532e-06      	-2.485153221e-01\n"
"# Region type :  patch freesurface\n"
"# Faces          : 15044\n"
"# Area           : 1.015103911e-02\n"
"# Scale factor   : 1.000000000e+00\n"
"# Time           	sum(phi)	sum(phiv)\n"
"0.0266967        	-2.829626314e-01	-3.275030457e-04\n"
"0.0266969        	-2.829588836e-01	-3.274987079e-04\n"
    );

    std::vector<std::string> names;
    auto result = readSingleTabularFile(tmp / "data.dat", -1, "()", &names);
    const auto& m = result.at("default");

    // Final layout from last header: Time sum(phi) sum(phiv) — 3 columns
    insight::assertion(m.n_rows == 4 && m.n_cols == 3,
        "mid-file-insertion: unexpected matrix shape");
    insight::assertion(names.size() == 3,
        "mid-file-insertion: expected 3 column names");
    insight::assertion(names[0] == "Time",      "mid-file-insertion: col0");
    insight::assertion(names[1] == "sum(phi)",  "mid-file-insertion: col1");
    insight::assertion(names[2] == "sum(phiv)", "mid-file-insertion: col2");

    // Early rows: sum(phiv) absent → 0.0; sum(phi) must stay in col 1
    checkClose(m(0, 0), 6.61511e-07,        "mid-file-insertion: row0 Time");
    checkClose(m(0, 1), -2.696333733e-01,   "mid-file-insertion: row0 sum(phi)");
    checkClose(m(0, 2), 0.0,                "mid-file-insertion: row0 sum(phiv) (absent)");
    checkClose(m(1, 1), -2.485153221e-01,   "mid-file-insertion: row1 sum(phi)");
    checkClose(m(1, 2), 0.0,                "mid-file-insertion: row1 sum(phiv) (absent)");

    // Later rows: all columns present
    checkClose(m(2, 1), -2.829626314e-01,   "mid-file-insertion: row2 sum(phi)");
    checkClose(m(2, 2), -3.275030457e-04,   "mid-file-insertion: row2 sum(phiv)");
    checkClose(m(3, 1), -2.829588836e-01,   "mid-file-insertion: row3 sum(phi)");
    checkClose(m(3, 2), -3.274987079e-04,   "mid-file-insertion: row3 sum(phiv)");

    fs::remove_all(tmp);
    std::cout << "PASS: test_midFileColumnInsertion" << std::endl;
}

// Multiple comment lines in a header block: only the last one is the label row.
static void test_multiLineHeaderBlock()
{
    auto tmp = makeTempDir();
    writeFile(tmp / "data.dat",
        "# OpenFOAM surface field value output\n"
        "# Time p T\n"
        "0.1 1.0 300.0\n"
    );

    std::vector<std::string> names;
    auto result = readSingleTabularFile(tmp / "data.dat", -1, "()", &names);
    const auto& m = result.at("default");

    insight::assertion(m.n_rows == 1 && m.n_cols == 3,
        "multi-line-header: unexpected matrix shape");
    insight::assertion(names[1] == "p", "multi-line-header: col1 should be 'p'");
    insight::assertion(names[2] == "T", "multi-line-header: col2 should be 'T'");

    fs::remove_all(tmp);
    std::cout << "PASS: test_multiLineHeaderBlock" << std::endl;
}

// filterChars is applied to data lines but NOT to header lines.
// A column named "p(Pa)" must survive with its brackets intact.
static void test_filterCharsNotAppliedToHeader()
{
    auto tmp = makeTempDir();
    writeFile(tmp / "data.dat",
        "# Time p(Pa) T\n"
        "0.1 (1.0) 300.0\n"
    );

    std::vector<std::string> names;
    auto result = readSingleTabularFile(tmp / "data.dat", -1, "()", &names);
    const auto& m = result.at("default");

    insight::assertion(names[1] == "p(Pa)",
        "filter-chars: column name brackets must be preserved");
    // Brackets stripped from data → value 1.0
    checkClose(m(0, 1), 1.0, "filter-chars: p(Pa) data value");

    fs::remove_all(tmp);
    std::cout << "PASS: test_filterCharsNotAppliedToHeader" << std::endl;
}

// Column removed in a later segment: earlier rows retain values,
// later rows get 0.0 for the removed column.
static void test_midFileColumnRemoval()
{
    auto tmp = makeTempDir();
    writeFile(tmp / "data.dat",
        "# Time p T\n"
        "0.1 1.0 300.0\n"
        "0.2 2.0 310.0\n"
        "# Time p\n"          // T removed
        "0.3 3.0\n"
        "0.4 4.0\n"
    );

    std::vector<std::string> names;
    auto result = readSingleTabularFile(tmp / "data.dat", -1, "()", &names);
    const auto& m = result.at("default");

    // Final layout: Time p (2 columns — last header wins)
    insight::assertion(m.n_rows == 4 && m.n_cols == 2,
        "mid-file-removal: unexpected matrix shape");
    insight::assertion(names.size() == 2, "mid-file-removal: expected 2 col names");
    insight::assertion(names[0] == "Time", "mid-file-removal: col0");
    insight::assertion(names[1] == "p",    "mid-file-removal: col1");

    checkClose(m(0, 1), 1.0, "mid-file-removal: row0 p");
    checkClose(m(1, 1), 2.0, "mid-file-removal: row1 p");
    checkClose(m(2, 1), 3.0, "mid-file-removal: row2 p");
    checkClose(m(3, 1), 4.0, "mid-file-removal: row3 p");

    fs::remove_all(tmp);
    std::cout << "PASS: test_midFileColumnRemoval" << std::endl;
}


// -----------------------------------------------------------------------
// Cross-file tests  (readAndCombineTabularFiles)
//
// Directory layout under caseLocation:
//   postProcessing/testFO/0/data.dat   (for OFversion >= 170)
//   postProcessing/testFO/1/data.dat
// or
//   testFO/0/data.dat                  (for OFversion < 170)
//   testFO/1/data.dat
//
// The clippedTable() logic uses strict inequalities (t > A && t < clipB),
// so test rows must not fall exactly on the boundary times.
// Time dir 0: t in {0.0, 0.5, 0.9, 1.0}  →  after clipping at A(td1)=1.0:
//             rows returned are t=0.5 and t=0.9
// Time dir 1: t in {1.0, 1.5, 2.0, 2.5}  →  not clipped:
//             rows returned are t=1.5 and t=2.0
// -----------------------------------------------------------------------

static fs::path foBasePath(const OpenFOAMCase& cm,
                            const fs::path& caseLocation,
                            const std::string& foName)
{
    if (cm.OFversion() < 170)
        return caseLocation / foName;
    else
        return caseLocation / "postProcessing" / foName;
}

// Cross-file test 1: inserted column across restart
// Time dir 0: "Time p T"     →  Time dir 1: "Time p pTotal T"
static void test_crossFile_insertedColumn(const OpenFOAMCase& cm,
                                           const fs::path& caseLocation)
{
    const std::string foName = "crossfile_insert";
    auto foBase = foBasePath(cm, caseLocation, foName);

    // Time dir 0
    fs::create_directories(foBase / "0");
    writeFile(foBase / "0" / "data.dat",
        "# Time p T\n"
        "0.0 10.0 100.0\n"
        "0.5 20.0 200.0\n"
        "0.9 30.0 300.0\n"
        "1.0 40.0 400.0\n"
    );
    // The OverlappingIntervals map is keyed by file write time (time_t, 1-second
    // resolution). Both files must have different timestamps or the second insert
    // overwrites the first, losing td0 data entirely.
    std::time_t now = std::time(nullptr);
    fs::last_write_time(foBase / "0" / "data.dat", now - 10);

    // Time dir 1 (restart, new column pTotal inserted before T)
    fs::create_directories(foBase / "1");
    writeFile(foBase / "1" / "data.dat",
        "# Time p pTotal T\n"
        "1.0  50.0 55.0 500.0\n"
        "1.5  60.0 65.0 600.0\n"
        "2.0  70.0 75.0 700.0\n"
        "2.5  80.0 85.0 800.0\n"
    );
    fs::last_write_time(foBase / "1" / "data.dat", now);

    auto result = readAndCombineTabularFiles(
        cm, caseLocation, foName, "data.dat", "()", "");

    // Final layout (from newest file): Time p pTotal T  →  4 columns
    // td0 clipped at 1.0: [0.0,1.0) → rows t=0.0,0.5,0.9  (3 rows)
    // td1 unclipped:      [1.0,∞)   → rows t=1.0,1.5,2.0,2.5  (4 rows)
    // Combined: 7 rows
    insight::assertion(result.n_cols == 4,
        "cross-insert: expected 4 columns");
    insight::assertion(result.n_rows == 7,
        "cross-insert: expected 7 rows");

    // td0 rows: pTotal absent → 0.0; T must be in col 3
    checkClose(result(0, 0), 0.0,   "cross-insert: row0 Time");
    checkClose(result(0, 1), 10.0,  "cross-insert: row0 p");
    checkClose(result(0, 2), 0.0,   "cross-insert: row0 pTotal (absent)");
    checkClose(result(0, 3), 100.0, "cross-insert: row0 T");

    checkClose(result(1, 0), 0.5,   "cross-insert: row1 Time");
    checkClose(result(1, 1), 20.0,  "cross-insert: row1 p");
    checkClose(result(1, 2), 0.0,   "cross-insert: row1 pTotal (absent)");
    checkClose(result(1, 3), 200.0, "cross-insert: row1 T");

    checkClose(result(2, 1), 30.0,  "cross-insert: row2 p");
    checkClose(result(2, 2), 0.0,   "cross-insert: row2 pTotal (absent)");
    checkClose(result(2, 3), 300.0, "cross-insert: row2 T");

    // td1 rows: all columns present
    checkClose(result(3, 0), 1.0,   "cross-insert: row3 Time");
    checkClose(result(3, 1), 50.0,  "cross-insert: row3 p");
    checkClose(result(3, 2), 55.0,  "cross-insert: row3 pTotal");
    checkClose(result(3, 3), 500.0, "cross-insert: row3 T");

    checkClose(result(4, 0), 1.5,   "cross-insert: row4 Time");
    checkClose(result(4, 1), 60.0,  "cross-insert: row4 p");
    checkClose(result(4, 2), 65.0,  "cross-insert: row4 pTotal");
    checkClose(result(4, 3), 600.0, "cross-insert: row4 T");

    checkClose(result(5, 1), 70.0,  "cross-insert: row5 p");
    checkClose(result(5, 2), 75.0,  "cross-insert: row5 pTotal");
    checkClose(result(5, 3), 700.0, "cross-insert: row5 T");

    checkClose(result(6, 0), 2.5,   "cross-insert: row6 Time");
    checkClose(result(6, 1), 80.0,  "cross-insert: row6 p");
    checkClose(result(6, 2), 85.0,  "cross-insert: row6 pTotal");
    checkClose(result(6, 3), 800.0, "cross-insert: row6 T");

    std::cout << "PASS: test_crossFile_insertedColumn" << std::endl;
}

// Cross-file test 2: identical column layout across files → behaviour unchanged
static void test_crossFile_sameLayout(const OpenFOAMCase& cm,
                                       const fs::path& caseLocation)
{
    const std::string foName = "crossfile_same";
    auto foBase = foBasePath(cm, caseLocation, foName);

    std::time_t now = std::time(nullptr);

    fs::create_directories(foBase / "0");
    writeFile(foBase / "0" / "data.dat",
        "# Time p T\n"
        "0.0 10.0 100.0\n"
        "0.5 20.0 200.0\n"
        "0.9 30.0 300.0\n"
        "1.0 40.0 400.0\n"
    );
    fs::last_write_time(foBase / "0" / "data.dat", now - 10);

    fs::create_directories(foBase / "1");
    writeFile(foBase / "1" / "data.dat",
        "# Time p T\n"
        "1.0 50.0 500.0\n"
        "1.5 60.0 600.0\n"
        "2.0 70.0 700.0\n"
        "2.5 80.0 800.0\n"
    );
    fs::last_write_time(foBase / "1" / "data.dat", now);

    auto result = readAndCombineTabularFiles(
        cm, caseLocation, foName, "data.dat", "()", "");

    // 3 columns, 7 rows: td0 [0.0,1.0) → 0.0,0.5,0.9; td1 [1.0,∞) → 1.0,1.5,2.0,2.5
    insight::assertion(result.n_cols == 3,
        "cross-same: expected 3 columns");
    insight::assertion(result.n_rows == 7,
        "cross-same: expected 7 rows");

    checkClose(result(0, 1), 10.0,  "cross-same: row0 p");
    checkClose(result(0, 2), 100.0, "cross-same: row0 T");
    checkClose(result(1, 1), 20.0,  "cross-same: row1 p");
    checkClose(result(1, 2), 200.0, "cross-same: row1 T");
    checkClose(result(2, 1), 30.0,  "cross-same: row2 p");
    checkClose(result(2, 2), 300.0, "cross-same: row2 T");
    checkClose(result(3, 1), 50.0,  "cross-same: row3 p");
    checkClose(result(3, 2), 500.0, "cross-same: row3 T");
    checkClose(result(4, 1), 60.0,  "cross-same: row4 p");
    checkClose(result(4, 2), 600.0, "cross-same: row4 T");
    checkClose(result(5, 1), 70.0,  "cross-same: row5 p");
    checkClose(result(5, 2), 700.0, "cross-same: row5 T");
    checkClose(result(6, 1), 80.0,  "cross-same: row6 p");
    checkClose(result(6, 2), 800.0, "cross-same: row6 T");

    std::cout << "PASS: test_crossFile_sameLayout" << std::endl;
}


// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main()
{
    try
    {
        // --- Within-file tests (no OpenFOAM environment required) ---
        test_noHeader();
        test_singleHeader();
        test_midFileColumnInsertion();
        test_multiLineHeaderBlock();
        test_filterCharsNotAppliedToHeader();
        test_midFileColumnRemoval();

        // --- Cross-file tests (require an OpenFOAM environment) ---
        OpenFOAMCase cm;
        auto caseLocation = makeTempDir();

        test_crossFile_insertedColumn(cm, caseLocation);
        test_crossFile_sameLayout(cm, caseLocation);

        fs::remove_all(caseLocation);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
