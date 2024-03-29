#include "cacheableentityhashes.h"
#include "base/linearalgebra.h"

namespace boost {


std::size_t hash<arma::mat>::operator()
  (const arma::mat& v) const
{
    return std::hash<arma::mat>()(v);
}



std::size_t hash<boost::filesystem::path>::operator()
  (const boost::filesystem::path& fn) const
{
    return std::hash<boost::filesystem::path>()(fn);
}


std::size_t hash<const vtkPolyData&>::operator()
    (const vtkPolyData& cv) const
{
  vtkPolyData& v = const_cast<vtkPolyData&>(cv);

  size_t h=0;
  boost::hash_combine(h, boost::hash<vtkIdType>()(v.GetNumberOfPoints()));
  boost::hash_combine(h, boost::hash<vtkIdType>()(v.GetNumberOfCells()));
  auto np=v.GetNumberOfPoints();
  auto step=std::max<vtkIdType>(1, np/4);
  for (vtkIdType i=0; i<np; i+=step)
  {
    double x[3];
    v.GetPoint(i, x);
    for (int j=0; j<3; ++j)
      boost::hash_combine(h, boost::hash<double>()(x[j]));
  }
  return h;
}


void writeStats(std::ostream& os, const vtkDataSet& pd)
{
    vtkDataSet& v = const_cast<vtkDataSet&>(pd);

    double bounds[6];
    vtkPointSet::SafeDownCast(&v)->GetBounds(bounds);

    os << v.GetNumberOfPoints() << " points, "<<v.GetNumberOfCells()<<" cells,"
          " bound from ("<<bounds[0]<<", "<<bounds[2]<<", "<<bounds[4]<<")"
          " to ("<<bounds[1]<<", "<<bounds[3]<<", "<<bounds[5]<<")";
}



std::size_t hash<vtkSmartPointer<vtkPolyData> >::operator()
    (const vtkSmartPointer<vtkPolyData>& v) const
{
    return hash<const vtkPolyData&>()(*v);
}

std::size_t hash<std::vector<vtkSmartPointer<vtkPolyData> > >::operator()
  (const std::vector<vtkSmartPointer<vtkPolyData> >& v) const
{
  size_t h=0;
  for (const auto& pd: v)
    boost::hash_combine(h, boost::hash<vtkSmartPointer<vtkPolyData> >()(pd));
  return h;
}

}
