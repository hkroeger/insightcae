#ifndef INSIGHT_SNAPPYHEXMESHOUTPUTANALYZER_H
#define INSIGHT_SNAPPYHEXMESHOUTPUTANALYZER_H

#include "base/outputanalyzer.h"
#include "base/progressdisplayer.h"
#include <memory>
#include "base/boost_include.h"

namespace insight {

class snappyHexMeshOutputAnalyzer
    : public OutputAnalyzer
{

  std::shared_ptr<ActionProgress> bmp_;

  int extrudedFaces_, totalFaces_;
  double extrudedFraction_;

  bool rmp1=false, inLayer=false;
  std::string layerIter, illegal;

  boost::regex read1, read2,
      suref1, suref2,
      shref1, shref2,
      rmp, rmpres,
      morph, morphiter,
      ladd, laddit, licheck,
      lisum1, lisum2, lisum3,
      write
      ;

public:
  snappyHexMeshOutputAnalyzer(ProgressDisplayer* parentProgress=nullptr);

  void update(const std::string& line) override;

  inline int totalFaces() const { return totalFaces_; }
  inline double extrudedFraction() const { return extrudedFraction_; }
};

} // namespace insight

#endif // INSIGHT_SNAPPYHEXMESHOUTPUTANALYZER_H
