#include "cacheableentityhashes.h"



namespace boost {


std::size_t hash<arma::mat>::operator()
  (const arma::mat& v) const
{
  std::hash<double> dh;
  size_t h=0;
  for (arma::uword i=0; i<v.n_elem; i++)
  {
    boost::hash_combine(h, dh(v(i)));
  }
  return h;
}



std::size_t hash<boost::filesystem::path>::operator()
  (const boost::filesystem::path& fn) const
{
  size_t h=0;
  // build from file path string and last write time (latter only if file exists)
  boost::hash_combine(h, fn.string());
  if (boost::filesystem::exists(fn))
  {
    boost::hash_combine(h, boost::filesystem::last_write_time(fn));
  }
  return h;
}

std::size_t hash<vtkSmartPointer<vtkPolyData> >::operator()
  (const vtkSmartPointer<vtkPolyData>& v) const
{
  size_t h=0;
  boost::hash_combine(h, boost::hash<vtkIdType>()(v->GetNumberOfPoints()));
  boost::hash_combine(h, boost::hash<vtkIdType>()(v->GetNumberOfCells()));
  auto np=v->GetNumberOfPoints();
  auto step=std::max<vtkIdType>(1, np/4);
  for (vtkIdType i=0; i<np; i+=step)
  {
    double x[3];
    v->GetPoint(i, x);
    for (int j=0; j<3; ++j)
      boost::hash_combine(h, boost::hash<double>()(x[j]));
  }
  return h;
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
