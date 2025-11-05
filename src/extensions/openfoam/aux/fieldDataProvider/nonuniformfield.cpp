#include "nonuniformfield.h"

namespace Foam {


template<class T>
void nonuniformField<T>::appendInstant(Istream& is)
{
    token firstToken(is);
    if (firstToken.isWord() && firstToken.wordToken()=="file")
    {
        fileName infname(is);
        IFstream inf(infname);
        values_.push_back(new Field<T>(inf));
    }
    else
    {
        is.putBack(firstToken);
        values_.push_back(new Field<T>(is));
    }
}

template<class T>
void nonuniformField<T>::writeInstant(int i, Ostream& os) const
{
    values_[i].UList<T>::
#if OF_VERSION>=060000 //defined(OFesi1806)
        writeList
#else
        writeEntry
#endif
        (os);
}


template<class T>
nonuniformField<T>::nonuniformField(Istream& is)
    : FieldDataProvider<T>(is)
{}

template<class T>
nonuniformField<T>::nonuniformField(const nonuniformField<T>& o)
    : FieldDataProvider<T>(o),
    values_(o.values_)
{
}

template<class T>
nonuniformField<T>::nonuniformField(const Field<T>& uf)
    : FieldDataProvider<T>()
{
    FieldDataProvider<T>::timeInstants_.resize(1);
    FieldDataProvider<T>::timeInstants_[0]=0;

    values_.clear();
    values_.push_back(new Field<T>(uf));
}

template<class T>
tmp<Field<T> > nonuniformField<T>::atInstant(int i, const pointField& target) const
{
    tmp<Field<T> > res(new Field<T>(values_[i]));
    return res;
}

template<class T>
autoPtr<FieldDataProvider<T> > nonuniformField<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new nonuniformField<T>(*this));
}

template<class T>
void nonuniformField<T>::autoMap
    (
        const fvPatchFieldMapper& m
        )
{
    for (size_t i=0; i<values_.size(); i++)
    {
        values_[i].autoMap(m);
    }
}


//- Reverse map the given fvPatchField onto this fvPatchField
template<class T>
void nonuniformField<T>::rmap
    (
        const FieldDataProvider<T>& o,
        const labelList& m
        )
{
    const nonuniformField<T>* oo = dynamic_cast<const nonuniformField<T>* >(&o);
    if (oo->values_.size() != values_.size())
    {
        FatalErrorIn("nonuniformField<T>::rmap")
        << "Incompatible number of time instants!"
        <<" other: "<<label(oo->values_.size())
        <<" current: "<<label(values_.size())
        << endl << abort(FatalError);
    }
    for (size_t i=0; i<values_.size(); i++)
    {
        values_[i].rmap( oo->values_[i], m );
    }
}


} // namespace Foam
