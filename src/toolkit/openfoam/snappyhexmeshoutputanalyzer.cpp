#include "snappyhexmeshoutputanalyzer.h"

#include "base/progressdisplayer.h"
#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace insight {

snappyHexMeshOutputAnalyzer::snappyHexMeshOutputAnalyzer(ProgressDisplayer* parentProgress)
    : OutputAnalyzer(parentProgress),
      bmp_( parentProgress?
            std::make_shared<ActionProgress>(parentProgress->forkNewAction(9, "snappyHexMesh"))
            : nullptr
           )
{}


void snappyHexMeshOutputAnalyzer::update(const std::string& line)
{
    boost::smatch match;

    boost::regex
        read1("Read mesh in = (.+) s"),
        read2("Read features in = (.+) s"),

        suref1("Surface refinement iteration (.+)"),
        suref2("After refinement surface refinement iteration (.+) : cells:([^ ]+)  faces:([^ ]+)  points:([^ ]+)"),

        shref1("Shell refinement iteration (.+)"),
        shref2("After refinement shell refinement iteration (.+) : cells:([^ ]+)  faces:([^ ]+)  points:([^ ]+)"),

        rmp("Remove unreachable sections of mesh"),
        rmpres("After subsetting : cells:([^ ]+)  faces:([^ ]+)  points:([^ ]+)"),

        morph("Morphing phase"),
        morphiter("Morph iteration (.+)"),

        ladd("Shrinking and layer addition phase"),
        laddit("Layer addition iteration (.+)"),
        licheck("Checking mesh with layer ..."),

        lisum1("Detected ([^ ]+) illegal faces .*"),
        lisum2("Extruding ([^ ]+) out of ([^ ]+) faces .*"),
        lisum3("Added ([^ ]+) out of ([^ ]+) cells ([^ ]+)%."),

        write("Layers added in = (.+) s.");
        ;



    try
    {
        if ( bmp_ && boost::regex_search(line, match, read1, boost::match_default) )
        {
          bmp_->message(match[0]);
        }
        else if ( bmp_ && boost::regex_search(line, match, read2, boost::match_default) )
        {
          bmp_->stepUp();
          bmp_->message(match[0]);
        }
        else if ( bmp_ && boost::regex_search(line, match, suref1, boost::match_default) )
        {
          if (match[1]=="1") bmp_->stepUp();
        }
        else if ( bmp_ && boost::regex_search(line, match, suref2, boost::match_default) )
        {
          bmp_->message("Surface refinement #"+match[1]+": "+match[2]+"cells");
        }
        else if ( bmp_ && boost::regex_search(line, match, shref1, boost::match_default) )
        {
          if (match[1]=="1") bmp_->stepUp();
        }
        else if ( bmp_ && boost::regex_search(line, match, shref2, boost::match_default) )
        {
          bmp_->message("Shell refinement #"+match[1]+": "+match[2]+"cells");
        }
        else if (bmp_ &&  boost::regex_search(line, match, rmp, boost::match_default) )
        {
            if (!rmp1) { bmp_->stepUp(); rmp1=true; }
            bmp_->message(match[0]);
        }
        else if ( bmp_ && boost::regex_search(line, match, rmpres, boost::match_default) )
        {
            bmp_->message("Removed unreachable cells, now "+match[1]+" remain");
        }
        else if ( bmp_ && boost::regex_search(line, match, morph, boost::match_default) )
        {
          bmp_->stepUp();
          bmp_->message(match[0]);
        }
        else if (bmp_ &&  boost::regex_search(line, match, morphiter, boost::match_default) )
        {
            bmp_->message("Morph iteration "+match[1]);
        }

        else if ( boost::regex_search(line, match, ladd, boost::match_default) )
        {
          if (bmp_)
          {
            bmp_->stepUp();
            bmp_->message(match[0]);
          }
          inLayer=true;
        }
        else if ( inLayer && boost::regex_search(line, match, laddit, boost::match_default) )
        {
            layerIter=match[1];
        }
        else if ( inLayer && boost::regex_search(line, match, lisum1, boost::match_default) )
        {
            illegal=match[1];
        }
        else if ( inLayer && boost::regex_search(line, match, lisum2, boost::match_default) )
        {
            extrudedFaces_=lexical_cast<int>(match[1]);
            totalFaces_=lexical_cast<int>(match[2]);
            extrudedFraction_=0;
            if (totalFaces_>0) extrudedFraction_=double(extrudedFaces_)/double(totalFaces_);
            if (bmp_)
              bmp_->message(
                    "Layer iter #"+layerIter+": illegal "+illegal+
                    ", extruded "+match[1]+" out of "+match[2]+" ("+lexical_cast<string>(100.*extrudedFraction_)+"%"
                  );

        }
    }
    catch (...)
    {
      // ignore errors
    }
}

} // namespace insight
