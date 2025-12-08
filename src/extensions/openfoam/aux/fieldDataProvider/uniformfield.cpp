#include "uniformfield.h"

namespace Foam {



template<class T>
uniformField<T>::uniformField(Istream& is)
    : FieldDataProvider<T>(is)
{
}

template<class T>
void uniformField<T>::appendInstant(Istream& is)
{
    T v;
    is >> v;
    values_.push_back(new T(v));
}

template<class T>
void uniformField<T>::writeInstant(int i, Ostream& is) const
{
    is << values_[i];
}

template<class T>
tmp<Field<T> > uniformField<T>::atInstant(int i, const pointField& target) const
{
    tmp<Field<T> > res(new Field<T>(target.size()));
    UNIOF_TMP_NONCONST(res)=values_[i];
    return res;
}

template<class T>
uniformField<T>::uniformField(const uniformField<T>& o)
    : FieldDataProvider<T>(o),
    values_(o.values_)
{
}

template<class T>
uniformField<T>::uniformField(const T& uv)
    : FieldDataProvider<T> ()
{
    FieldDataProvider<T>::timeInstants_.resize(1);
    FieldDataProvider<T>::timeInstants_[0]=0;

    values_.clear();
    values_.push_back(new T(uv));
}

template<class T>
autoPtr<FieldDataProvider<T> > uniformField<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new uniformField<T>(*this));
}



} // namespace Foam
