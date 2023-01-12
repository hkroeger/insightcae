#include "radsoftwareenvironment.h"

namespace insight {

RADSoftwareEnvironment::RADSoftwareEnvironment()
{}

boost::filesystem::path RADSoftwareEnvironment::preprocessLSDynaInput(
        const boost::filesystem::path &caseFile,
        int np,
        OutputAnalyzer *oa)
{
    auto job = forkCommand(
                "starter_linux64_gf",
                {
                    "-i", caseFile.string(),
                    "-np", boost::lexical_cast<std::string>(np),
                    "-nt", "1"
                } );

    job->ios_run_with_interruption(oa);
    job->wait();

    if (job->process().exit_code()!=0)
        throw insight::Exception("OpenRadioss preprocessor: external command execution failed with nonzero exit code!");

    // check for success
    auto cdir = caseFile.parent_path();
    auto cfn = caseFile.filename().stem().string();

    auto radFile=cdir/(cfn+"_0001.rad");

    insight::assertion(
                boost::filesystem::exists(radFile),
                "no radioss input file was created" );

    return radFile;
}

void RADSoftwareEnvironment::runSolver(
        const boost::filesystem::path &radFile,
        int np,
        OutputAnalyzer *oa )
{
    JobPtr job;

    if (np==1)
    {
        job = forkCommand(
                "engine_linux64_gf",
                {
                    "-i", radFile.string()
                } );
    }
    else
    {
        job = forkCommand(
                "mpirun",
                {
                    "-np", boost::lexical_cast<std::string>(np),
                    "engine_linux64_gf_ompi",
                    "-i", radFile.string(),
                    "-nt", "1"
                } );
    }

    job->ios_run_with_interruption(oa);
    job->wait();

    if (job->process().exit_code()!=0)
        throw insight::Exception("OpenRadioss::runSolver(): external command execution failed with nonzero exit code!");

}

boost::filesystem::path RADSoftwareEnvironment::anim2VTK(
        const boost::filesystem::path &animFile )
{
    auto outfilename = animFile.parent_path() /
            (animFile.filename().stem().string()+".vtk");

    std::vector<std::string> output;
    executeCommand(
                "anim_to_vtk_linux64_gf",
                { animFile.string() },
                &output,
                nullptr,
                false, true );

    {
        std::ofstream f(outfilename.string());
        std::copy(
            output.begin(),
            output.end(),
            std::ostream_iterator<std::string>(f, "\n")
            );
    }
    boost::filesystem::remove(animFile);

    return outfilename;
}

} // namespace insight
