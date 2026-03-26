
#include "base/boost_include.h"

#include "base/cppextensions.h"
#include <boost/algorithm/string/case_conv.hpp>




namespace boost
{
namespace filesystem
{




template < >
// path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt)
path&
path::append< typename path::iterator >(
    typename path::iterator begin,
    typename path::iterator end )
{
    for( ; begin != end ; ++begin )
        *this /= *begin;
    return *this;
}




// Return path when appended to a_From will resolve to same as a_To
path
make_relative(
    path a_From,
    path a_To )
{
    a_From = absolute( a_From ); a_To = absolute( a_To );
    path ret;
    path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );

    // Find common base
    for(
        path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ;
        itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo;
        ++itrFrom, ++itrTo );

    // Navigate backwards in directory to reach previously found base
    for( path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
    {
        if( (*itrFrom) != "." )
            ret /= "..";
    }

    // Now navigate down the directory branch
    ret.append( itrTo, a_To.end() );
    return ret;
}




// from Rob Kennedy
// https://stackoverflow.com/users/33732/rob-kennedy
bool path_contains_file(path dir, path file)
{
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
    // If dir ends with "/" and isn't the root directory, then the final
    // component returned by iterators will include "." and will interfere
    // with the std::equal check below, so we strip it before proceeding.
    if (dir.filename() == ".")
        dir.remove_filename();
    // We're also not interested in the file's name.
    assert(file.has_filename());
    file.remove_filename();

    // If dir has more components than file, then file can't possibly
    // reside in dir.
    auto dir_len = std::distance(dir.begin(), dir.end());
    auto file_len = std::distance(file.begin(), file.end());
    if (dir_len > file_len)
        return false;

    // This stops checking when it reaches dir.end(), so it's OK if file
    // has more directory components afterward. They won't be checked.
    return std::equal(dir.begin(), dir.end(), file.begin());
}




bool weakly_equivalent(path first, path second)
{
    auto fullpath1=weakly_canonical(absolute(first));
    auto fullpath2=weakly_canonical(absolute(second));

    return fullpath1 == fullpath2;
}

bool is_executable(const path &p)
{
#ifdef WIN32
    return to_lower_copy(p.extension().string())==".exe";
#else
    auto perms = status(p).permissions();
    return (perms & (owner_exe | group_exe | others_exe)) != no_perms;
#endif
}




}
}




namespace std
{

std::size_t hash<boost::filesystem::path>::operator()
    (const boost::filesystem::path& fn) const
{
    size_t h=boost::filesystem::hash_value(fn);
    // build from file path string and last write time (latter only if file exists)
    if (boost::filesystem::exists(fn))
    {
        std::hash_combine(h, boost::filesystem::last_write_time(fn));
    }
    return h;
}

}
