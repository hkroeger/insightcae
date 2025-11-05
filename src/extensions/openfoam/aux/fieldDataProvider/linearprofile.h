#ifndef LINEARPROFILE_H
#define LINEARPROFILE_H

#include "fielddataprovider.h"

namespace Foam {


template<class T>
class linearProfile
    : public FieldDataProvider<T>
{
    LinearVectorSpaceBase base_;
    std::vector<fileName> filenames_;
    mutable boost::ptr_map<int, insight::Interpolator> values_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    //- Runtime type information
    TypeName("linearProfile");

    linearProfile(Istream& is);
    linearProfile(const linearProfile<T>& o);

    virtual void read(Istream& is);
    virtual void writeSup(Ostream& os) const;

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
    virtual autoPtr<FieldDataProvider<T> > clone() const;
};


} // namespace Foam

#ifdef NoRepository
#   include "linearprofile.cpp"
#endif

#endif // LINEARPROFILE_H
