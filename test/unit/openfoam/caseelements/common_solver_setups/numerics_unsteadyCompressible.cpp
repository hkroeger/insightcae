#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/unsteadycompressiblenumerics.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

      insight::assertion(argc==2, "expected exactly one command line argument");

      class Case : public OpenFOAMCaseWithCylinderMesh
      {

      public:
        Case(const string& OFEname) : OpenFOAMCaseWithCylinderMesh(OFEname) {}

        void createCaseElements() override
        {
            CompressiblePIMPLESettings::Parameters ti;
            ti.pressure_velocity_coupling=
                    CompressiblePIMPLESettings::Parameters::pressure_velocity_coupling_PIMPLE_type
                    {1, 1, 1e-3, 1e-3, 0.2, 0.5, 0.5, 0.7, true};
            ti.timestep_control=
                    CompressiblePIMPLESettings::Parameters::timestep_control_fixed_type{};

            unsteadyCompressibleNumerics::Parameters p;
            p.time_integration=ti;
            p.pinternal=1e5;
            p.endTime=1;
            p.deltaT=1.;
            p.endTime=1.;

            insert(new unsteadyCompressibleNumerics(*this, p) );
            insert(new compressibleSinglePhaseThermophysicalProperties(*this));
            insert(new kOmegaSST_RASModel(*this));
        }
      } tc(argv[1]);

      try
      {
        tc.runTest();
      }
      catch (insight::ExternalProcessFailed& ex)
      {
        bool ok=false;
        if ((tc.OFversion()==164) && (ex.exeName()=="rhoPimpleFoam"))
        {
            // this will always segfault because of error in leastSquares-related destructor
            if (boost::filesystem::is_directory(tc.dir()/"1"))
            {
                ok=true;
            }
        }
        if (!ok) throw ex;
      }
  });
}
