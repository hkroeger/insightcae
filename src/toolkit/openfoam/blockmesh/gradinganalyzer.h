#ifndef INSIGHT_BMD_GRADINGANALYZER_H
#define INSIGHT_BMD_GRADINGANALYZER_H


namespace insight {
namespace bmd {




class GradingAnalyzer
{
  double grad_;
public:
  GradingAnalyzer(double grad);

  /**
   * compute grading from cell size at beginning and end
   */
  GradingAnalyzer(double delta0, double delta1);

  /**
   * grading from condition: minimum cell length delta0 on edge of length L discretized with n cells
   */
  GradingAnalyzer(double delta0, double L, int n);

  inline double grad() const { return grad_; }

  int calc_n(double delta0, double L) const;
  double calc_L(double delta0, int n) const;
  double calc_delta0(double L, int n) const;
  double calc_delta1(double delta0) const;
};




} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_GRADINGANALYZER_H
