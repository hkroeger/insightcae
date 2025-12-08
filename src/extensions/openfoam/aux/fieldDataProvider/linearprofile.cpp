#include "linearprofile.h"

namespace Foam {




template<class T>
linearProfile<T>::linearProfile(Istream& is)
    : FieldDataProvider<T>(is)
{
}

template<class T>
void linearProfile<T>::appendInstant(Istream& is)
{
    fileName fn;
    is >> fn;
    filenames_.push_back(fn);
}

template<class T>
void linearProfile<T>::writeInstant(int i, Ostream& is) const
{
    is << filenames_[i];
}

template<class T>
tmp<Field<T> > linearProfile<T>::atInstant(int idx, const pointField& target) const
{
    if (values_.find(idx)==values_.end())
    {
        fileName fn=filenames_[idx];
        arma::mat xy;
        fn.expand();
        xy.load(fn.c_str(), arma::raw_ascii);
        std::auto_ptr<insight::Interpolator> newipol(new insight::Interpolator(xy, true));
        values_.insert(idx, newipol);
    }

    tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
    Field<T>& res=UNIOF_TMP_NONCONST(resPtr);

    forAll(target, pi)
    {
        double t = base_.t(target[pi]);
        arma::mat q = (*values_.find(idx)->second)(t);

        for (size_t c=0; c<q.n_elem; c++)
        {
            setComponent( res[pi], c ) = q(c);
        }
        res[pi]=base_(res[pi]);
    }

    return resPtr;
}

template<class T>
linearProfile<T>::linearProfile(const linearProfile<T>& o)
    : FieldDataProvider<T>(o),
    base_(o.base_),
    filenames_(o.filenames_),
    values_(o.values_)
{
}

template<class T>
void linearProfile<T>::read(Istream& is)
{
    base_.read(is);
    FieldDataProvider<T>::read(is);
}

template<class T>
void linearProfile<T>::writeSup(Ostream& os) const
{
    base_.writeSup(os);
}

template<class T>
autoPtr<FieldDataProvider<T> > linearProfile<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new linearProfile<T>(*this));
}



} // namespace Foam
