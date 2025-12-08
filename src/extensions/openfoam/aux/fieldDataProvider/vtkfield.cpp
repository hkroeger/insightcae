#include "vtkfield.h"

#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPolyDataReader.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkProbeFilter.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkCompositeDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkDataObject.h"
#include "vtkUnstructuredGrid.h"

#include "vtkconversion.h"

namespace Foam {




template<class T>
void vtkField<T>::setComponentMap(const word& n)
{
    componentOrderName_=n;
    // default: no interchange
    componentMap_.clear();
    for (int k=0; k<pTraits<T>::nComponents; ++k)
        componentMap_.push_back(k);
}




template<class T>
void vtkField<T>::appendInstant(Istream& is)
{
    fileName fn;
    string fld;

    is >> fn >> fld;

    fn.expand();

    autoPtr<token> nextToken;
    if (!is.eof()) nextToken.reset(new token(is));

    if (nextToken.valid() && nextToken->isWord() && nextToken->wordToken()=="componentOrder" )
    {
        word orderType;
        is >> orderType;

        setComponentMap(orderType);
    }
    else
    {
        if (nextToken.valid())
            is.putBack(nextToken());

        setComponentMap();
    }

    vtkFiles_.push_back(fn);
    fieldNames_.push_back(fld);
}




template<class T>
void vtkField<T>::writeInstant(int i, Ostream& os) const
{
    os << vtkFiles_[i]
       << token::SPACE
       << fieldNames_[i]
        ;
    if (!componentOrderName_.empty())
    {
        os << token::SPACE << "componentOrder"
           << token::SPACE << componentOrderName_
            ;
    }
}




template<class T>
vtkField<T>::vtkField(Istream& is)
    : FieldDataProvider<T>(is)
{}




template<class T>
vtkField<T>::vtkField(const vtkField<T>& o)
    : FieldDataProvider<T> (o),
    vtkFiles_(o.vtkFiles_),
    fieldNames_(o.fieldNames_),
    componentMap_(o.componentMap_)
{}




template<class T>
tmp<Field<T> > vtkField<T>::atInstant(int i, const pointField& target) const
{
    auto ii=data_.find(i);
    if (ii==data_.end())
    {
        auto fn=vtkFiles_[i];
        if (!exists(fn))
        {
            FatalErrorIn("vtkField<T>::atInstant")
            << "file "<<vtkFiles_[i]<<" does not exist!"
            <<abort(FatalError);
        }

        if (fn.ext()=="vtm")
        {
            auto r = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
            r->SetFileName(fn.c_str());
            r->Update();
            data_[i] = multiBlockDataSetToUnstructuredGrid(r->GetOutput());
        }
        else
        {
            auto r = vtkSmartPointer<vtkGenericDataObjectReader>::New();
            r->SetFileName(fn.c_str());
            r->Update();
            data_[i] = r->GetOutput();
        }
        ii=data_.find(i);
    }


    auto targ = vtkSmartPointer<vtkPolyData>::New();
    setPoints<vtkPolyData>(target, targ);

    auto ip=vtkSmartPointer<vtkProbeFilter>::New();
    ip->SetInputData(targ);
    ip->SetSourceData(ii->second);

    ip->Update();

    auto vpm = ip->GetValidPoints();
    int validPts=vpm->GetSize();

    if (validPts==0)
    {
        WarningIn("vtkField<T>::atInstant")
        << "no point could be interpolated!" << endl;
    }
    else
    {
        Info<<"Successfully interpolated "<<validPts<<" of "<<int(targ->GetNumberOfPoints())<<" points."<<endl;
    }

    auto out = ip->GetOutput();
    if (!out->GetPointData()->HasArray(fieldNames_[i].c_str()))
    {
        FatalErrorIn("vtkField<T>::atInstant")
        << "file "<<vtkFiles_[i]<<" does not contain field "
        << fieldNames_[i] <<"!"
        <<abort(FatalError);
    }
    return VTKArrayToField<T>(
        out->GetPointData()->GetArray(fieldNames_[i].c_str()),
        componentMap_ );
}




template<class T>
autoPtr<FieldDataProvider<T> > vtkField<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new vtkField<T>(*this));
}


} // namespace Foam
