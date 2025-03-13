
#include "cadparametersetvisualizer.h"
#include "airfoilsection.h"


namespace insight
{


insight::CADParameterSetModelVisualizer::IconFunctions::Add<AirfoilSection>
    addAirfoilSectionIcon(
        insight::CADParameterSetModelVisualizer::iconForAnalysis_table(),
        []() {
            return QIcon(":/analysis_airfoil2d.svg");
        } );

insight::CADParameterSetModelVisualizer::IconFunctions::Add<AirfoilSectionPolar>
    addAirfoilSectionPolarIcon(
        insight::CADParameterSetModelVisualizer::iconForAnalysis_table(),
        []() {
            return QIcon(":/analysis_airfoil2d_vs_alpha.svg");
        } );


}


struct TestCasesResInit {
    TestCasesResInit() {
        Q_INIT_RESOURCE(testcases);
    }
    ~TestCasesResInit() {
        Q_CLEANUP_RESOURCE(testcases);
    }
} testCasesResInit;
