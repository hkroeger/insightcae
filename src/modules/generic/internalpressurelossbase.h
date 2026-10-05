#ifndef INTERNALPRESSURELOSSBASE_H
#define INTERNALPRESSURELOSSBASE_H

#include "base/exception.h"
#include "base/units.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamparameterstudy.h"
#include "openfoam/caseelements/evaluation/calculatetotalpressure.h"
#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/basic/porouszone.h"

#include "internalpressurelossbase_pdl.h"


namespace insight {



class InternalPressureLossBase
    : public insight::OpenFOAMAnalysis
{
public:
    static void modifyDefaults(ParameterSet& p);
#include INTERNALPRESSURELOSSBASE_PDL_InternalPressureLossBase
/*
PARAMETERSET>>>

inherits OpenFOAMAnalysis::Parameters


geometry =  labeledarray "geometry_%d" [ set {

    file = cadgeometry "" "Part of the geometry. May be an STL file or CAD exchange format (STEP or IGES)." *necessary

    lm = int -1 "Minimum refinement level. If value is negative, the global minimum refinement level is used."
    lx = int -1 "Maximum refinement level. If value is negative, the global maximum refinement level is used."

    role = selectablesubset {{

        refinementOnly set {
            mode = selection ( inside outside distance ) inside "Refinement mode"
            dist = double 1e15 "Maximum distance for refinement. Set very large, if mode is inside." *necessary
        }

        wall set {
            roughness_z0 = double 0 "Wall roughness height"
        }

        symmetry set {
        }

        inlet set {
            specification = selectablesubset {{
              vector set {
                velocity = vector (0 0 0) ""
              }
              massFlow set {
                dotm = double 0.001 "[kg/s] mass flow"
              }
              volumetricFlow set {
                Q = double 0.001 "[m^3/s] volume flow"
              }
              pressureInlet set {
                ambientPressure = double 0. ""
              }
            }} vector "type of velocity specification"
        }

        outlet set {
           pressure = double 0. "pressure difference to ambient pressure at the outlet"
        }

        porousVolume set {
            d = double 0. "[Pa/m^2] darcy contribution" *necessary
            f = double 0. "[kg/m^3] forchheimer contribution" *necessary
        }

    }} wall "Boundary role of the geometry"

} ] *1
"Pieces of geometry.
 All pieces together must completely resemble the perimeter of the internal channel
 (except for the porousVolume)."





geometryscale = double 1e-3     "scaling factor to scale geometry files to meters"



mesh=set
{
  size		= double	10 		"[mm] Cell size of template mesh." *necessary
  minLevel      = int           0               "Minimum refinement level on geometry."
  maxLevel      = int           2               "Maximum refinement level on geometry."
  nLayers       = int           3               "Number of prism layers"
  tlayer= double 0.5 "Layer thickness value"
  erlayer = double 1.3 "Expansion ratio of layers"
  relativeSizes = bool true "Whether tlayer specifies relative thickness (absolute thickness if set to false)"
  PiM           = vector        (0 0 0)         "Seed point inside flow domain." *necessary
} "Properties of the computational mesh"




eval = set {
 averageFraction = double 0.2
"fraction of the the total simulation duration,
 over which the iteration history of each quantity is averaged to obtain the reported figure."

 additionalCutPlaneLocations = labeledarray "cutplane_%d" [
   vector (0 0 0) ""
 ] *0
} "Parameters for evaluation"


<<<PARAMETERSET
*/


    struct supplementedInputData
        : public supplementedInputDataDerived<Parameters>
    {
    public:
        supplementedInputData(
            ParameterSetInput ip,
            const boost::filesystem::path& workDir,
            ActionProgress& progress );

        Supplemented<BoundingBox> bb_;
        Supplemented<arma::mat> L_;

        Supplemented<int> nx_, ny_, nz_;

        // cad::FeaturePtr inlet_, outlet_;
        // std::map<std::string, cad::FeaturePtr> walls_;

        boost::filesystem::path stldir_;
        std::string fn_inlet_, fn_outlet_;
    };

    addParameterMembers_SupplementedInputData(Parameters);


public:
    declareType("Internal Pressure Loss Base");

    InternalPressureLossBase(
        const std::shared_ptr<supplementedInputDataBase>& sp );

    typedef
        std::pair<std::set<std::string>, std::set<std::string> >
            InOutPatches;
    InOutPatches findInOutPatches() const;

    void calcDerivedInputData(ProgressDisplayer& parentActionProgress) override;

    virtual std::unique_ptr<OpenFOAMCaseElement> createWallBC(
        insight::OpenFOAMCase& cm, const std::string& patchName, const OFDictData::dict& boundaryDict) const;
    virtual double inletTemperature(const std::string& patchName) const;
    virtual std::unique_ptr<OpenFOAMCaseElement> createInletBC(
        insight::OpenFOAMCase& cm, const std::string& patchName,
        const Parameters::geometry_default_type::role_inlet_type* in,
        const OFDictData::dict& boundaryDict) const;

    struct SolverProperties
    {
        enum Phi { Mass, Volume } phi;
        /**
         * @brief phiFactor
         * factor to convert phi to mass phi. 1 for phi==Mass, constRho for phi==Volume
         */
        double phiFactor;
        enum Pressure { Kinematic, NonKinematic } pressure;
        /**
         * @brief phiFactor
         * factor to convert p to non-kinematic pressure. 1 for pressure==NonKinematic, 1./constRho for pressure==Kinematic
         */
        double pressureFactor;
    };

    virtual SolverProperties solverProperties() const  =0;

    virtual std::unique_ptr<calculateTotalPressure::Parameters>
    totalPressureCalculationParameters() const;

    virtual double ambientPressure() const;

    virtual std::unique_ptr<porousZoneOption::Parameters>
    porousZoneParameters(
        const std::string& label,
        const Parameters::geometry_default_type::role_porousVolume_type* vol) const =0;

    virtual std::unique_ptr<FVNumerics> numericsCaseElement(OpenFOAMCase& cm) const =0;

    void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    void createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    void applyCustomPreprocessing(OpenFOAMCase& cm, ProgressDisplayer& progress) override;


    struct RenderingScene
    {
        OpenFOAMCaseScene scene;
        InOutPatches in_out;
        vtkSmartPointer<vtkUnstructuredGridAlgorithm>
            inlet, outlet, patches, internal;
        std::vector<std::string> wallPatchNames;

        arma::mat bb, L;
        double Lmax;

        CoordinateSystem objCS;
        std::map<std::string, View> views;

        vtkCamera *camera;

        RenderingScene(
            const boost::filesystem::path& exePath,
            const OFDictData::dict& boundaryDict,
            InOutPatches inOutPatches,
            std::function<const Parameters&(void)> p,
            std::function<const supplementedInputData&(void)> sp
            );
    };

    ResultSetPtr evaluateResults(OpenFOAMCase& cmp, ProgressDisplayer& parentActionProgress) override;
};



} // namespace insight

#endif // INTERNALPRESSURELOSSBASE_H
