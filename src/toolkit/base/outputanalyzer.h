#ifndef INSIGHT_OUTPUTANALYZER_H
#define INSIGHT_OUTPUTANALYZER_H

#include <string>

namespace insight {

class ProgressDisplayer;

class OutputAnalyzer
{
protected:
  ProgressDisplayer* progress_;

public:
  OutputAnalyzer(ProgressDisplayer* progress=nullptr);
  virtual ~OutputAnalyzer();

  virtual void update(const std::string& line) =0;

  virtual bool stopRun() const;
};

} // namespace insight

#endif // INSIGHT_OUTPUTANALYZER_H
