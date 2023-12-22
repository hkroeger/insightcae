#ifndef CACHEABLEENTITYHASHES_H
#define CACHEABLEENTITYHASHES_H

#include <vector>
#include <armadillo>

#include "base/boost_include.h"
#include <boost/functional/hash.hpp>

#undef AllValues
#include <limits>
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

namespace boost
{

template<> struct hash<arma::mat>
{
  std::size_t operator()(const arma::mat& v) const;
};

template<> struct hash<boost::filesystem::path>
{
  std::size_t operator()(const boost::filesystem::path& fn) const;
};

template<> struct hash<const vtkPolyData& >
{
    std::size_t operator()(const vtkPolyData& v) const;
};

void writeStats(std::ostream& os, const vtkDataSet& pd);

template<> struct hash<vtkSmartPointer<vtkPolyData> >
{
  std::size_t operator()(const vtkSmartPointer<vtkPolyData>& v) const;
};

template<> struct hash<std::vector<vtkSmartPointer<vtkPolyData> > >
{
  std::size_t operator()(const std::vector<vtkSmartPointer<vtkPolyData> >& v) const;
};

} // namespace boost



#endif // CACHEABLEENTITYHASHES_H
