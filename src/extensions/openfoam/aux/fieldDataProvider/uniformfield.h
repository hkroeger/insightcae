#ifndef UNIFORMFIELD_H
#define UNIFORMFIELD_H

#include "fielddataprovider.h"

namespace Foam {

template<class T>
class uniformField
    : public FieldDataProvider<T>
{
    boost::ptr_vector<T> values_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    //- Runtime type information
    TypeName("uniform");

    uniformField(Istream& is);
    uniformField(const uniformField<T>& o);
    uniformField(const T& uv);

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
    virtual autoPtr<FieldDataProvider<T> > clone() const;
};


} // namespace Foam

#ifdef NoRepository
#   include "uniformfield.cpp"
#endif

#endif // UNIFORMFIELD_H
