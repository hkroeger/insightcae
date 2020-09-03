#include "blockmeshoutputanalyzer.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace insight {

BlockMeshOutputAnalyzer::BlockMeshOutputAnalyzer(ProgressDisplayer* parentProgress, int nb)
    : OutputAnalyzer(parentProgress),
      nBlocks_(nb),
      bmp_( parentProgress?
            std::make_shared<ActionProgress>(parentProgress->forkNewAction(nb+4, "blockMesh"))
            : nullptr
           )
{
}

void BlockMeshOutputAnalyzer::update(const std::string& line)
{
    if (bmp_)
    {
        boost::smatch match;

        boost::regex check_pattern("Check topology");
        boost::regex bl_pattern("Block (.+) cell size :");
        boost::regex bo_pattern("Creating block offsets");
        boost::regex cc_pattern("Creating cells");
        boost::regex wp_pattern("^Writing polyMesh");
        boost::regex end_pattern("^End");
        try
        {
            if ( boost::regex_search(line, match, check_pattern, boost::match_default) )
            {
                bmp_->stepUp();
                bmp_->message("Checking topology");
            }
            else if ( boost::regex_search(line, match, bl_pattern, boost::match_default) )
            {
                bmp_->stepUp();
                bmp_->message("Analyzed block "+match[1]);
            }
            else if ( boost::regex_search(line, match, bo_pattern, boost::match_default) )
            {
                bmp_->stepUp();
                bmp_->message("Creating block offsets");
            }
            else if ( boost::regex_search(line, match, cc_pattern, boost::match_default) )
            {
                bmp_->stepUp();
                bmp_->message("Creating cells");
            }
            else if ( boost::regex_search(line, match, wp_pattern, boost::match_default) )
            {
                bmp_->stepUp();
                bmp_->message("Writing mesh");
            }
            else if ( boost::regex_search(line, match, end_pattern, boost::match_default) )
            {
                bmp_->message("Finished");
                bmp_.reset();
            }
        }
        catch (...)
        {
          // ignore errors
        }
    }
}

} // namespace insight
