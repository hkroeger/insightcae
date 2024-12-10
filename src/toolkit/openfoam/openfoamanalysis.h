/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#ifndef INSIGHT_OPENFOAMANALYSIS_H
#define INSIGHT_OPENFOAMANALYSIS_H

#include "base/analysis.h"

#include "boost/thread/detail/thread.hpp"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/turbulencemodel.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/progressdisplayer/convergenceanalysisdisplayer.h"

#include "openfoam/caseelements/basic/decomposepardict.h"
#include "openfoam/caseelements/turbulencemodel.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/ofes.h"
#include "base/progressdisplayer/combinedprogressdisplayer.h"
#include "base/progressdisplayer/convergenceanalysisdisplayer.h"
#include "base/progressdisplayer/prefixedprogressdisplayer.h"

#include "openfoam/caseelements/basic/rasmodel.h"

#include "openfoamtools.h"

#include "base/boost_include.h"


#include "openfoamanalysis__OpenFOAMAnalysis__Parameters_headers.h"


#include <libintl.h>
#pragma push_macro("_")
#undef _
#define _(String) dgettext("toolkit", String)


namespace insight {



class ConvergenceAnalysisDisplayer;

int realNp(int userInputNp);

template<class Base>
class OpenFOAMAnalysisTemplate : public Base
{
public:

#include "openfoamanalysis__OpenFOAMAnalysis__Parameters.h"
/*
PARAMETERSET>>> OpenFOAMAnalysis Parameters
inherits Base::Parameters

run = set 
{	
 machine 	= 	string 	"" 	"Machine or queue, where the external commands are executed on. Defaults to 'localhost', if left empty." *hidden
 OFEname 	= 	string 	"OFesi1806" "Identifier of the OpenFOAM installation, that shall be used"
 np 		= 	int 	0 	"Number of processors for parallel run (1 means serial execution, <1 means that all available processors are used)" *necessary
 mapFrom 	= 	path 	"" 	"Map solution from specified case, if not empty. potentialinit is skipped if specified."
 potentialinit 	= 	bool 	false 	"Whether to initialize the flow field by potentialFoam when no mapping is done" *hidden
 evaluateonly	= 	bool 	false 	"Whether to skip solver run and do only the evaluation"
 preprocessonly	= 	bool 	false 	"Whether to only prepare the case and stop before launching the solver" *hidden
} "Execution parameters"

mesh = set
{
 linkmesh 	= 	path 	"" 	"If not empty, the mesh will not be generated, but a symbolic link to the polyMesh folder of the specified OpenFOAM case will be created." *hidden
} "Properties of the computational mesh"

eval = set
{
 reportdicts 	= 	bool 	false 	"Include dictionaries into report" *hidden
 skipmeshquality 	= 	bool 	false 	"Check to exclude mesh check during evaluation" *hidden
} "Parameters for evaluation after solver run"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

  ResultSetPtr derivedInputData_;

  std::vector<std::shared_ptr<ConvergenceAnalysisDisplayer> > convergenceAnalysis_;

public:
    OpenFOAMAnalysisTemplate
        (
            const std::string& name,
            const std::string& description,
            const ParameterSet& ps,
            const boost::filesystem::path& exepath,
            ProgressDisplayer& progress = consoleProgressDisplayer
        )
      : Base(name, description, ps, exepath, progress),
        p_(ps)
    {}


    template<class PT>
    OpenFOAMAnalysisTemplate
    (
        const std::string& name,
        const std::string& description,
        std::unique_ptr<PT> sp,
        const boost::filesystem::path& exepath,
        ProgressDisplayer& progress = consoleProgressDisplayer
        )
      : Base(name, description, std::move(sp), exepath, progress),
        p_(Base::parameters())
    {}

    int np() const
    {
        return realNp(p_.run.np);
    }

    static insight::OperatingSystemSet compatibleOperatingSystems()
    {
        return { insight::LinuxOS };
    }

    boost::filesystem::path setupExecutionEnvironment() override
    {
        CurrentExceptionContext ex("creating the execution directory");

        return Analysis::setupExecutionEnvironment();
    }
    
    void initializeDerivedInputDataSection()
    {
        if (!derivedInputData_)
        {
            ParameterSet empty_ps;
            derivedInputData_.reset(new ResultSet(empty_ps, "Derived Input Data", ""));
        }
    }

    virtual void reportIntermediateParameter(const std::string& name, double value, const std::string& description="", const std::string& unit="")
    {
        initializeDerivedInputDataSection();

        std::cout<<">>> Intermediate parameter "<<name<<" = "<<value<<" "<<unit;
        if (description!="")
            std::cout<<" ("<<description<<")";
        std::cout<<std::endl;

        boost::assign::ptr_map_insert<ScalarResult>(*derivedInputData_) (name, value, description, "", unit);
    }
    
    virtual void calcDerivedInputData(ProgressDisplayer& progress)
    {}

    virtual void createMesh(OpenFOAMCase& cm, ProgressDisplayer& progress) =0;
    virtual void createCase(OpenFOAMCase& cm, ProgressDisplayer& progress) =0;
    
    virtual void createDictsInMemory(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
    {
        CurrentExceptionContext ex(
            _("creating OpenFOAM dictionaries in memory for case \"%s\""),
            this->executionPath().c_str() );

        dicts=cm.createDictionaries();
    }
    
    virtual void writeDictsToDisk(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
    {
        CurrentExceptionContext ex(
            _("writing OpenFOAM dictionaries to case \"%s\""),
            this->executionPath().c_str() );

        cm.createOnDisk(this->executionPath(), dicts);
        cm.modifyCaseOnDisk(this->executionPath());
    }

    /**
     * Customize dictionaries before they get written to disk
     */
    virtual void applyCustomOptions(OpenFOAMCase& cm, std::shared_ptr<OFdicts>& dicts)
#ifdef SWIG
;
#else
    {
        CurrentExceptionContext ex(
            _("applying custom options to OpenFOAM case configuration for case \"%s\""),
            this->executionPath().c_str() );


        OFDictData::dict& dpd=dicts->lookupDict("system/decomposeParDict");
        if (dpd.find("numberOfSubdomains")!=dpd.end())
        {
            int cnp=boost::get<int>(dpd["numberOfSubdomains"]);
            if (cnp!=np())
            {
                insight::Warning
                    (
                        "decomposeParDict does not contain proper number of processors!\n"
                        +str(boost::format("(%d != %d)\n") % cnp % np())
                        +"It will be recreated but the directional preferences cannot be taken into account.\n"
                          "Correct this by setting the np parameter in FVNumerics during case creation properly."
                        );
                decomposeParDict(cm, decomposeParDict::Parameters()
                                         .set_np(np())
                                         .set_decompositionMethod(decomposeParDict::Parameters::decompositionMethod_type::scotch)
                                 ).addIntoDictionaries(*dicts);
            }
        }
    }
#endif


    
    /**
     * Do modifications to the case when it has been created on disk
     */
    virtual void applyCustomPreprocessing(OpenFOAMCase& cm, ProgressDisplayer& progress)
    {}
    



    void changeMapFromPath(const boost::filesystem::path& newMapFromPath)
    {
        p_.run.mapFrom->setOriginalFilePath(newMapFromPath);
    }




    virtual void mapFromOther(
        OpenFOAMCase& cm,
        ProgressDisplayer& parentAction,
        const boost::filesystem::path& mapFromPath,
        bool is_parallel)
#ifdef SWIG
;
#else
    {
        CurrentExceptionContext ex(
            _("mapping existing CFD solution from case \"%s\" to case \"%s\""),
            mapFromPath.c_str(), this->executionPath().c_str() );

        if (const RASModel* rm=cm.get<RASModel>(".*"))
        {
            // check, if turbulence model is compatible in source case
            // run "createTurbulenceFields" on source case, if not
            try
            {
                OpenFOAMCase oc(cm.ofe());
                std::string omodel=readTurbulenceModelName(oc, mapFromPath);
                if ( (rm->type()!=omodel) && (omodel!="kOmegaSST2"))
                {
                    CurrentExceptionContext ex("converting turbulence quantities in case \""+mapFromPath.string()+"\" since the turbulence model is different.");
                    parentAction.message(ex);
                    oc.executeCommand(mapFromPath, "createTurbulenceFields", { "-latestTime" } );
                }
            }
            catch (...)
            {
                insight::Warning("could not check turbulence model of source case. Continuing anyway.");
            }
        }

        parentAction.message("Executing mapFields");
        mapFields(cm, mapFromPath, this->executionPath(), is_parallel, cm.fieldNames());
    }
#endif



    virtual void installConvergenceAnalysis(std::shared_ptr<ConvergenceAnalysisDisplayer> cc)
    {
        convergenceAnalysis_.push_back(cc);
    }
    


    
    /**
     * @brief prepareCaseCreation
     * perform action prior to actual case setup,
     * e.g. running another solver to obtain corrections for settings
     * @param progress
     */
    virtual void prepareCaseCreation(ProgressDisplayer& progress)
    {}




    /**
     * integrate all steps before the actual run
     */
    virtual void createCaseOnDisk(OpenFOAMCase& runCase, ProgressDisplayer& parentActionProgress)
#ifdef SWIG
;
#else
    {
        auto dir = this->executionPath();

        CurrentExceptionContext ex(
            _("creating OpenFOAM case in directory \"%s\""),
            dir.c_str() );

        OFEnvironment ofe = OFEs::get(p_.run.OFEname);
        ofe.setExecutionMachine(p_.run.machine);

        parentActionProgress.message(_("Computing derived input quantities"));
        calcDerivedInputData(parentActionProgress);

        bool evaluateonly=p_.run.evaluateonly;
        if (evaluateonly)
        {
            insight::Warning(
                _("Parameter \"run/evaluateonly\" is set.\nSKIPPING SOLVER RUN AND PROCEEDING WITH EVALUATION!")
                );
        }

        std::shared_ptr<OpenFOAMCase> meshCase;
        bool meshcreated=false;
        if (!evaluateonly)
        {
            meshCase.reset(new OpenFOAMCase(ofe));
            if (!meshCase->meshPresentOnDisk(dir))
            {
                meshcreated=true;
                if (p_.mesh.linkmesh->isValid())
                {
                    parentActionProgress.message(
                        str(boost::format(
                                _("Linking the mesh to OpenFOAM case in directory %s.")
                                ) % dir.string() ) );
                    linkPolyMesh(p_.mesh.linkmesh->filePath(dir)/"constant", dir/"constant", &ofe);
                }
                else
                {
                    parentActionProgress.message(_("Creating the mesh."));
                    createMesh(*meshCase, parentActionProgress);
                }
            }
            else
            {
                insight::Warning(
                    _("case in \"%s\": mesh is already there, skipping mesh creation."),
                    dir.c_str() );
            }
        }

        int np=1;
        if (boost::filesystem::exists(dir/"system"/"decomposeParDict"))
        {
            np=readDecomposeParDict(dir);
        }
        bool is_parallel = np>1;

        if (!runCase.outputTimesPresentOnDisk(dir, is_parallel) && !evaluateonly)
        {
            runCase.modifyFilesOnDiskBeforeDictCreation( dir );
        }


        parentActionProgress.message(_("Creating the case setup."));
        createCase(runCase, parentActionProgress);

        std::shared_ptr<OFdicts> dicts;

        parentActionProgress.message(_("Creating the dictionaries."));
        createDictsInMemory(runCase, dicts);

        parentActionProgress.message(_("Applying custom modifications to dictionaries."));
        applyCustomOptions(runCase, dicts);

        // might have changed
        if (boost::filesystem::exists(dir/"system"/"decomposeParDict"))
        {
            np=readDecomposeParDict(dir);
        }
        is_parallel = np>1;

        if (!runCase.outputTimesPresentOnDisk(dir, is_parallel) && !evaluateonly)
        {
            if (meshcreated)
            {
                parentActionProgress.message(_("Applying custom modifications mesh."));
                runCase.modifyMeshOnDisk(dir);
            }

            parentActionProgress.message(_("Writing dictionaries to disk."));
            writeDictsToDisk(runCase, dicts);

            parentActionProgress.message(_("Applying custom preprocessing steps to OpenFOAM case."));
            applyCustomPreprocessing(runCase, parentActionProgress);
        }
        else
        {
            insight::Warning(
                _("case in \"%s\": skipping case recreation because there are already output time directories present."),
                dir.c_str() );
        }
    }
#endif


    virtual void initializeSolverRun(ProgressDisplayer& parentProgress, OpenFOAMCase& cm)
#ifdef SWIG
;
#else
    {
        auto exepath = this->executionPath();

        CurrentExceptionContext ex(
            _("initializing solver run for case \"%s\""),
            exepath.c_str() );

        int np=readDecomposeParDict(exepath);
        bool is_parallel = np>1;


        if (!cm.outputTimesPresentOnDisk(exepath, false))
        {
            if ((cm.OFversion()>=230) && (p_.run.mapFrom->isValid()))
            {
                // parallelTarget option is not present in OF2.3.x
                mapFromOther(cm, parentProgress, p_.run.mapFrom->filePath(exepath), false);
            }
        }

        if (is_parallel)
        {
            if (!exists(exepath/"processor0"))
            {
                parentProgress.message(_("Executing decomposePar"));

                std::vector<std::string> opts;
                if (exists(exepath/"constant"/"regionProperties"))
                    opts={"-allRegions"};

                cm.executeCommand(exepath, "decomposePar", opts);
            }
        }

        if (!cm.outputTimesPresentOnDisk(exepath, is_parallel))
        {
            if ( (!(cm.OFversion()>=230)) && (p_.run.mapFrom->isValid()) )
            {
                mapFromOther(cm, parentProgress, p_.run.mapFrom->filePath(exepath), is_parallel);
            }
            else
            {
                if (p_.run.potentialinit)
                {
                    if (p_.run.mapFrom->isValid())
                    {
                        parentProgress.message(
                            str(boost::format(
                                _("case in \"%s\": solution was mapped from other case, skipping potentialFoam.")
                                    ) % exepath.string() ) );
                        insight::Warning(
                            _("A potentialFoam initialization was configured although a solution was mapped from another case.\n"
                              "The potentialFoam run was skipped in order not to destroy the mapped solution!")
                            );
                    }
                    else
                    {
                        parentProgress.message(_("Executing potentialFoam"));
                        runPotentialFoam(cm, exepath, np);
                    }
                }
            }
        }
        else
        {
            parentProgress.message(
                str(boost::format(
                        _("case in \"%s\": output timestep are already there, skipping initialization.")
                        ) % exepath.string() )
                );
        }
    }
#endif


    virtual void runSolver(ProgressDisplayer& parentProgress, OpenFOAMCase& cm)
#ifdef SWIG
;
#else
    {
        CurrentExceptionContext ex(_("running solver"));
        auto exepath = this->executionPath();

        CombinedProgressDisplayer cpd(CombinedProgressDisplayer::OR), conv(CombinedProgressDisplayer::AND);
        cpd.add(&parentProgress);
        cpd.add(&conv);

        for (auto& ca: this->convergenceAnalysis_)
        {
            conv.add(ca.get());
        }


        std::string solverName;
        double endTime;
        int np=readDecomposeParDict(exepath);

        {
            OFDictData::dict controlDict;
            std::ifstream cdf( (exepath/"system"/"controlDict").c_str() );
            readOpenFOAMDict(cdf, controlDict);
            solverName=controlDict.getString("application");
            endTime=controlDict.getDoubleOrInt("endTime");
        }

        SolverOutputAnalyzer analyzer(cpd, endTime);

        parentProgress.message(
            str(boost::format(_("Executing application %s until end time %g."))
                % solverName % endTime) );

        cm.runSolver(exepath, analyzer, solverName, np);
    }
#endif



    virtual void finalizeSolverRun(OpenFOAMCase& cm, ProgressDisplayer& parentAction)
#ifdef SWIG
;
#else
    {
        auto exepath = this->executionPath();
        CurrentExceptionContext ex(
            _("finalizing solver run for case \"%s\""),
            exepath.c_str() );

        int np=readDecomposeParDict(exepath);
        bool is_parallel = np>1;
        if (is_parallel)
        {
            if (exists(exepath/"processor0"))
            {
                if (checkIfReconstructLatestTimestepNeeded(cm, exepath))
                {
                    parentAction.message(_("Running reconstructPar for latest time step"));

                    std::vector<std::string> opts={"-latestTime"};
                    if (exists(exepath/"constant"/"regionProperties"))
                        opts.push_back("-allRegions");

                    cm.executeCommand(exepath, "reconstructPar", opts );
                }
                else
                {
                    parentAction.message(_("No reconstruct needed"));
                }
            }
            else
                insight::Warning(
                    _("A parallel run is configured, but not processor directory is present!\n"
                      "Proceeding anyway.") );
        }
    }
#endif



    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress)
#ifdef SWIG
;
#else
    {
        auto exepath = this->executionPath();
        CurrentExceptionContext ex(
            _("evaluating the results for case \"%s\""),
            exepath.c_str() );

        auto results = std::make_shared<ResultSet>(
            this->parameters(), this->name_, "Result Report");
        results->introduction() = this->description_;

        if (!p_.eval.skipmeshquality)
        {
            parentActionProgress.message("Generating mesh quality report");
            meshQualityReport(cm, exepath, results);
        }

        if (p_.eval.reportdicts)
        {
            parentActionProgress.message("Adding numerical settings to report");
            currentNumericalSettingsReport(cm, exepath, results);
        }

        if (derivedInputData_)
        {
            parentActionProgress.message("Inserting derived input quantities into report");
            std::string key(derivedInputData_->title());
            results->insert( key, derivedInputData_->clone() ) .setOrder(-1.);
        }

        return results;
    }
#endif



    ResultSetPtr operator()(ProgressDisplayer& progress = consoleProgressDisplayer ) override
#ifdef SWIG
;
#else
    {
        CurrentExceptionContext ex(_("running OpenFOAM analysis"));

        auto ofprg = progress.forkNewAction( p_.run.evaluateonly? 5 : 7 );

        ofprg.message(_("Creating execution environment"));
        setupExecutionEnvironment();
        ++ofprg;

        OFEnvironment ofe = OFEs::get(p_.run.OFEname);
        ofe.setExecutionMachine(p_.run.machine);

        ofprg.message(_("Preparing case creation"));
        prepareCaseCreation(ofprg);
        ++ofprg;

        OpenFOAMCase runCase(ofe);
        ofprg.message(_("Creating case on disk"));
        createCaseOnDisk(runCase, ofprg);
        ++ofprg;

        auto dir = this->executionPath();

        if (!p_.run.evaluateonly)
        {
            PrefixedProgressDisplayer iniprogdisp(
                &ofprg, "initrun",
                PrefixedProgressDisplayer::Prefixed,
                PrefixedProgressDisplayer::NoActionProgressPrefix
                );

            ofprg.message(_("Initializing solver run"));
            initializeSolverRun(iniprogdisp, runCase);
            ++ofprg;
        }

        if (!p_.run.evaluateonly && !p_.run.preprocessonly)
        {
            ofprg.message(_("Running solver"));
            runSolver(ofprg, runCase);
            ++ofprg;
        }

        ResultSetPtr results;
        if (!p_.run.preprocessonly)
        {
            ofprg.message(_("Finalizing solver run"));
            finalizeSolverRun(runCase, ofprg);
            ++ofprg;

            ofprg.message(_("Evaluating results"));
            results = evaluateResults(runCase, ofprg);
            ++ofprg;
        }
        else
        {
            // empty report containing only the input parameters
            results = std::make_shared<ResultSet>(
                this->parameters(), this->name_, "Result Report");
        }

        return results;
    }
#endif

};

// #ifndef SWIG
typedef OpenFOAMAnalysisTemplate<Analysis> OpenFOAMAnalysis;
#ifdef SWIG
%template(OpenFOAMAnalysis) OpenFOAMAnalysisTemplate<Analysis>;
#endif
// #else
// typedef OpenFOAMAnalysisTemplate OpenFOAMAnalysis;
// #endif


template<class P>
turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const P& tmp )
{
  CurrentExceptionContext ex(
        _("inserting turbulence model configuration into OpenFOAM case")
        );

  turbulenceModel* model = turbulenceModel::lookup(tmp.selection, cm, tmp.parameters);

  if (!model)
      throw insight::Exception(_("Unrecognized RASModel selection: %s"), tmp.selection);

  return cm.insert(model);
}




turbulenceModel* insertTurbulenceModel(OpenFOAMCase& cm, const SelectableSubsetParameter& ps );




}

#pragma pop_macro("_")

#endif // INSIGHT_OPENFOAMANALYSIS_H
