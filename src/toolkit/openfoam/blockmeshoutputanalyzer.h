#ifndef INSIGHT_BLOCKMESHOUTPUTANALYZER_H
#define INSIGHT_BLOCKMESHOUTPUTANALYZER_H

#include "base/outputanalyzer.h"
#include "base/progressdisplayer.h"


namespace insight
{

class BlockMeshOutputAnalyzer
 : public OutputAnalyzer
{
    int nBlocks_;
    std::shared_ptr<ActionProgress> bmp_;

public:
  BlockMeshOutputAnalyzer(ProgressDisplayer* parentProgress=nullptr, int nBlocks=1);

  void update(const std::string& line) override;
};

} // namespace insight

#endif // INSIGHT_BLOCKMESHOUTPUTANALYZER_H
