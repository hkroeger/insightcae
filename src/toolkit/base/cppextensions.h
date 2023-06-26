#ifndef CPPEXTENSIONS_H
#define CPPEXTENSIONS_H

#include <memory>
#include <functional>
#include "base/exception.h"

namespace boost { namespace filesystem {
class path;
size_t hash_value(const boost::filesystem::path&);
}}



namespace std
{


template<class TargetType, class SourceType>
std::unique_ptr<TargetType> dynamic_unique_ptr_cast(std::unique_ptr<SourceType> src)
{
  if (TargetType* to = dynamic_cast<TargetType*>(src.get()))
  {
    src.release();
    return std::unique_ptr<TargetType>(to);
  }
  else
    throw insight::Exception("Could not cast unique_ptr!");
}


template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}



template<class E>
struct hash<std::vector<E> >
{
    size_t operator()(const std::vector<E>& v) const
    {
        size_t h=std::hash<size_t>()(v.size());
        for (const auto& e: v)
        {
            std::hash_combine(h, e);
        }
        return h;
    }
};




#if __cplusplus <= 201103L

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif

// https://stackoverflow.com/a/51591178
template<class T>
bool operator==(const std::shared_ptr<T> &lhs, const std::weak_ptr<T> &rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

template<class T>
class comparable_weak_ptr : public std::weak_ptr<T>
{
public:
    template<class ...Args>
    comparable_weak_ptr(Args&&... addArgs)
        : std::weak_ptr<T>(std::forward<Args>(addArgs)...)
    {}

    // https://stackoverflow.com/a/51591178
    bool operator==(const std::shared_ptr<T>& lhs) const
    {
        return !lhs.owner_before(*this) && !this->owner_before(lhs);
    }

    bool operator<(const std::weak_ptr<T>& rhs) const
    {
        return std::owner_less<std::weak_ptr<T> >()(*this, rhs);
    }

};

}




#endif // CPPEXTENSIONS_H
