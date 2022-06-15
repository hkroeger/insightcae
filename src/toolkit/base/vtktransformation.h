#ifndef INSIGHT_VTKTRANSFORMATION_H
#define INSIGHT_VTKTRANSFORMATION_H

#include "base/boost_include.h"
#include "base/linearalgebra.h"
#include "base/spatialtransformation.h"

#include <functional>

namespace insight {



class vtk_ChangeCS
    : public vtk_Transformer
{
  arma::mat m_;
public:
  vtk_ChangeCS
  (
      arma::mat from_ex,
      arma::mat from_ez,
      arma::mat to_ex,
      arma::mat to_ez
  );

  /**
    * initialize from matrix 4x4 = 16 coefficients
    */
  vtk_ChangeCS
  (
      const double *coeffs
  );

  /**
    * initialize from callback function
    */
  vtk_ChangeCS
  (
      std::function<double(int, int)> init_func,
      int nrows=4, int idx_ofs=0
  );

  vtkSmartPointer<vtkPolyDataAlgorithm> apply_VTK_Transform(vtkSmartPointer<vtkPolyDataAlgorithm> in) const override;
};



vtkSmartPointer<vtkPolyDataAlgorithm>
readSTL
(
  const boost::filesystem::path& path,
  const vtk_TransformerList& trsf = vtk_TransformerList()
);




} // namespace insight

#endif // INSIGHT_VTKTRANSFORMATION_H
