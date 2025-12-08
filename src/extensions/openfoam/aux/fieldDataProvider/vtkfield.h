#ifndef VTKFIELD_H
#define VTKFIELD_H

#include "fielddataprovider.h"

#include "vtkSmartPointer.h"



namespace Foam {


template<class T>
class vtkField
    : public FieldDataProvider<T>
{
public:
    typedef std::vector<int> ComponentMap;

    static const ComponentMap VTKSymmTensorMap, OpenFOAMSymmTensorMap;

private:
    std::vector<fileName> vtkFiles_;
    std::vector<string> fieldNames_;
    ComponentMap componentMap_;
    word componentOrderName_;

    void setComponentMap(const word& mapSelection = word());

    mutable std::map<int, vtkSmartPointer<vtkDataObject> > data_;
    mutable std::map<long int, Field<T> > cache_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    //- Runtime type information
    TypeName("vtkField");

    vtkField(Istream& is);
    vtkField(const vtkField<T>& o);

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
    virtual autoPtr<FieldDataProvider<T> > clone() const;
};

template<>
void vtkField<symmTensor>::setComponentMap(const word& orderType);


} // namespace Foam

#ifdef NoRepository
#   include "vtkfield.cpp"
#endif

#endif // VTKFIELD_H
