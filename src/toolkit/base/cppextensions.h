#ifndef CPPEXTENSIONS_H
#define CPPEXTENSIONS_H

#include <memory>
#include "base/exception.h"

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


#if __cplusplus <= 201103L

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif


}

#endif // CPPEXTENSIONS_H
