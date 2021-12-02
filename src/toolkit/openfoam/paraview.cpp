
#include "openfoam/paraview.h"
#include "base/cppextensions.h"

namespace insight
{

std::string caseLabel(const boost::filesystem::path &caseDirectory)
{
    std::string caseLabel;
    auto cd = boost::filesystem::canonical(caseDirectory);
    caseLabel = cd.filename().string();
    for (int i=0; i<2; ++i)
    {
     cd=cd.parent_path();
     dbg()<<"cd="<<cd<<endl;
     if (cd.empty()) break;
     caseLabel = cd.filename().string()+"_"+caseLabel;
    }
    return caseLabel;
}




std::ostream& Paraview::createLoadScript()
{
    loadScript_=std::make_unique<TemporaryFile>(
                "load-PV-%%%%.py", caseDirectory_ );
    auto &os = loadScript_->stream();
    os<<
         "from paraview.simple import *\n"
         //"_DisableFirstRenderCameraReset()\n"
         ;
    return os;
}




Paraview::Paraview(
        const boost::filesystem::path &caseDirectory,
        const boost::filesystem::path &stateFile,
        bool batch,
        bool parallelProjection,
        bool rescale,
        bool onlyLatestTime,
        double fromTime, double toTime,
        const std::vector<std::string> &additionalClientArgs,
        const boost::filesystem::path &dataDirectory
        )
    : caseDirectory_( caseDirectory ),
      dataDirectory_( dataDirectory.empty()?caseDirectory:dataDirectory )
{
    std::ostream& ls = createLoadScript();

    if (stateFile.empty())
    {
        ls << str(boost::format(
    "ofs=[OpenFOAMReader(FileName='%s/system/controlDict', SkipZeroTime=False)]\n"
    "times=ofs[0].TimestepValues\n"
    "print(times)\n"
    ) % dataDirectory_.generic_path().string() );
    }
    else
    {
        ls << str(boost::format(
     "LoadState('%s',"
           "LoadStateDataFileOptions='Search files under specified directory',"
           "DataDirectory='%s/system')\n"
     "ofs=list(filter(lambda s: 'OpenFOAMReader' in str(type(s)), "
                     "GetSources().values()))\n"
     "print(ofs)\n"
     "alltimes=set()\n"
     "for o in ofs: alltimes=set.union(alltimes,o.TimestepValues)\n"
     "times=sorted(list(alltimes))\n"
     "print(times)\n"
    ) % stateFile.generic_path().string()
      % dataDirectory_.generic_path().string() );
    }

    ls <<
    "AnimationScene1 = GetAnimationScene()\n";

    if (batch)
    {
        ls <<
        "ftimes=None\n"
        "if (len(times)==0):\n"
        " ftimes=[0]\n"
        "else\n";
        if (onlyLatestTime)
        {
            ls <<
        " ftimes=[times[-1]]\n";
        }
        else
        {
            ls << str(boost::format(
        " ftimes=filter(lambda t:   t>=%g and  t<=%g, times)\n"
                          )       % fromTime % toTime );
        }
        ls <<
        "for curtime in ftimes:\n"
        "  AnimationScene1.AnimationTime = curtime\n";
        if (parallelProjection)
        {
            ls <<
        "  # bug in PV4.4: parallel projection is not restored from state file\n"
        "  for view in GetRenderViews():\n"
        "     view.CameraParallelProjection = 0\n"
        "  RenderAllViews()\n"
        "  for view in GetRenderViews():\n"
        "     view.CameraParallelProjection = 1\n"
            ;
        }
        else
        {
            ls <<
        "  RenderAllViews()\n";
        }
        if (rescale)
        {
            ls <<
        "  import math\n"
        "  for view in GetRenderViews():\n"
        "    reps = view.Representations\n"
        "    for rep in reps:\n"
        "     if hasattr(rep, 'Visibility') and rep.Visibility == 1 and hasattr(rep, 'MapScalars') and rep.MapScalars != '':\n"
        "      input = rep.Input\n"
        "      input.UpdatePipeline() #make sure range is up-to-date\n"
        "      rep.RescaleTransferFunctionToDataRange(False)\n"
        ;
        }
        ls <<
        "  layouts=GetLayouts()\n"
        "  for i,l in enumerate(sorted(layouts.keys(), key=lambda k: k[0])):\n"
        "    fname='"<<caseLabel(caseDirectory_)<<"_layout%%02d'%%(i)\n"
        ;
        if (onlyLatestTime) ls <<
        "    fname+='_latesttime.png'\n";
        else ls <<
        "    fname+='_t%%g.png'%%(curtime)\n";
        ls <<
        "    print('Writing', fname)\n"
        "    SaveScreenshot(fname, layout=layouts[l], magnification=1, quality=100)\n"
        ;

    }
    else // not batch
    {
        ls <<
        "AnimationScene1.AnimationTime = times[-1]\n";
    }
    loadScript_->closeStream();

    if (batch)
    {
        process_=std::make_unique<boost::process::child>(
                    boost::process::search_path("pvbatch"),
                    boost::process::args({
                       "--use-offscreen-rendering",
                       loadScript_->path().string()
                                         })
                    );
    }
    else
    {
        std::vector<std::string> pvargs={
    #ifndef WIN32
            "--title", caseLabel(caseDirectory_),
    #endif
            "--script="+loadScript_->path().string()
        };

        std::copy(additionalClientArgs.begin(), additionalClientArgs.end(),
                  std::inserter(pvargs, pvargs.begin()) );

        process_=std::make_unique<boost::process::child>(
                    boost::process::search_path("paraview"),
                    boost::process::args(pvargs)
                    );
    }

    if (!process_->running())
      throw insight::Exception("Failed to launch paraview!");
}








}
