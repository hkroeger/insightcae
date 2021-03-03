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


}
