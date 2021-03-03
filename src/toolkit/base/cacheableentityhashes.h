#ifndef CACHEABLEENTITYHASHES_H
#define CACHEABLEENTITYHASHES_H

#include <armadillo>

#include "base/boost_include.h"
#include <boost/functional/hash.hpp>

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

} // namespace boost



#endif // CACHEABLEENTITYHASHES_H
