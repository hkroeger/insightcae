#ifndef NONUNIFORMFIELD_H
#define NONUNIFORMFIELD_H

#include "fielddataprovider.h"

namespace Foam {


template<class T>
class nonuniformField
    : public FieldDataProvider<T>
{
    boost::ptr_vector<Field<T> > values_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    //- Runtime type information
    TypeName("nonuniform");

    nonuniformField(Istream& is);
    nonuniformField(const nonuniformField<T>& o);
    nonuniformField(const Field<T>& uf);

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
    virtual autoPtr<FieldDataProvider<T> > clone() const;

    //- Map (and resize as needed) from self given a mapping object
    void autoMap
        (
            const fvPatchFieldMapper&
            );


    //- Reverse map the given fvPatchField onto this fvPatchField
    void rmap
        (
            const FieldDataProvider<T>&,
            const labelList&
            );
};


} // namespace Foam


#ifdef NoRepository
#   include "nonuniformfield.cpp"
#endif

#endif // NONUNIFORMFIELD_H
