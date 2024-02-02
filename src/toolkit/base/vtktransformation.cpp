
#include "vtktransformation.h"
#include "base/exception.h"

#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkSTLReader.h>

namespace insight {


vtk_ChangeCS::vtk_ChangeCS
(
    arma::mat from_ex,
    arma::mat from_ez,
    arma::mat to_ex,
    arma::mat to_ez
)
{
  CurrentExceptionContext ec("Computing VTK transformation matrix to change "
                             "from CS with ex="+valueList_to_string(from_ex)+" and ez="+valueList_to_string(from_ez)+" "
                             "to CS with ex="+valueList_to_string(to_ex)+" and ez="+valueList_to_string(to_ez)+".");

  from_ez /= arma::norm(from_ez,2);
  from_ex = arma::cross(from_ez, arma::cross(from_ex, from_ez));
  from_ex /= arma::norm(from_ex, 2);
  arma::mat from_ey = arma::cross(from_ez, from_ex);
  from_ey /= arma::norm(from_ex, 2);

  to_ez /= arma::norm(to_ez,2);
  to_ex = arma::cross(to_ez, arma::cross(to_ex, to_ez));
  to_ex /= arma::norm(to_ex, 2);
  arma::mat to_ey = arma::cross(to_ez, to_ex);
  to_ey /= arma::norm(to_ey, 2);

  arma::mat matrix(3,3);
  // matrix from XOY  ToA2 :
  matrix.col(0) = to_ex;
  matrix.col(1) = to_ey;
  matrix.col(2) = to_ez;

  arma::mat MA1(3,3);
  MA1.col(0)=from_ex;
  MA1.col(1)=from_ey;
  MA1.col(2)=from_ez;

  m_=arma::zeros(4,4);
  m_(3,3)=1.;
  m_.submat(0,0,2,2) = matrix.t() * MA1;
}

vtk_ChangeCS::vtk_ChangeCS
(
    const double *coeffs
)
{
  m_=arma::zeros(4,4);

  int k=0;
  for (arma::uword i=0; i<4; i++)
  {
    for (arma::uword j=0; j<4; j++)
    {
      m_(i,j)=coeffs[k++];
    }
  }
}

vtk_ChangeCS::vtk_ChangeCS
(
    std::function<double(int, int)> init_func,
    int nrows,
    int idx_ofs
)
{
  m_=arma::zeros(4,4);
  m_(3,3)=1.;

  for (arma::uword i=0; i<nrows; i++)
  {
    for (arma::uword j=0; j<4; j++)
    {
      m_(i,j) = init_func(i+idx_ofs, j+idx_ofs);
    }
  }
}

vtkSmartPointer<vtkPolyDataAlgorithm> vtk_ChangeCS::apply_VTK_Transform(vtkSmartPointer<vtkPolyDataAlgorithm> in) const
{
  CurrentExceptionContext ec("Applying VTK transformation.");

  vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  tf->SetInputConnection(in->GetOutputPort());

  vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
  const double m[] ={
    m_(0,0), m_(0,1), m_(0,2), m_(0,3),
    m_(1,0), m_(1,1), m_(1,2), m_(1,3),
    m_(2,0), m_(2,1), m_(2,2), m_(2,3),
    m_(3,2), m_(3,2), m_(3,2), m_(3,3)
  };
  t->SetMatrix(m);

  tf->SetTransform(t);

  return tf;
}

vtkSmartPointer<vtkPolyDataAlgorithm>
readSTL
(
  const boost::filesystem::path& path,
  const vtk_TransformerList& trsf
)
{
  CurrentExceptionContext ce("Reading STL file "+path.string()+" using VTK reader");

  if (!boost::filesystem::exists(path))
    throw insight::Exception("file "+path.string()+" does not exist!");

  vtkSmartPointer<vtkSTLReader> stl = vtkSmartPointer<vtkSTLReader>::New();
  stl->SetFileName(path.string().c_str());

  std::vector<vtkSmartPointer<vtkPolyDataAlgorithm> > imr;
  vtkSmartPointer<vtkPolyDataAlgorithm> i0=stl;
  for (const auto& t: trsf)
  {
    imr.push_back(vtkSmartPointer<vtkPolyDataAlgorithm>( t->apply_VTK_Transform(i0) ));
    i0=imr.back();
  }

  return i0;
}


} // namespace insight
