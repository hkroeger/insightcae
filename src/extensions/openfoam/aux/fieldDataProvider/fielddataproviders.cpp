/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "fielddataprovider.h"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{


template<>
void vtkField<symmTensor>::setComponentMap(const word& orderType)
{
    componentOrderName_=orderType;
    if (orderType.empty() || orderType=="OpenFOAM")
    {
        componentMap_ = {0, 1, 2, 3, 4, 5};
    }
    else if (orderType=="VTK")
    {
        componentMap_ = {0, 3, 5, 1, 4, 2};
    }
    else
    {
        FatalErrorIn("void vtkField<symmTensor>::setComponentMap(const word& orderType)")
                << "unrecognized component map "<<orderType<<endl
                << "allowed: OpenFOAM or VTk"<<endl
                <<abort(FatalError);
    }
}



#define makeProviders(TT) \
makeFieldDataProviderType(TT, scalar);\
makeFieldDataProviderType(TT, vector);\
makeFieldDataProviderType(TT, symmTensor);\
makeFieldDataProviderType(TT, sphericalTensor);\
makeFieldDataProviderType(TT, tensor)

makeFieldDataProvider(scalar);
makeFieldDataProvider(vector);
makeFieldDataProvider(symmTensor);
makeFieldDataProvider(sphericalTensor);
makeFieldDataProvider(tensor);

makeProviders(uniformField);
makeProviders(nonuniformField);
makeProviders(linearProfile);
makeProviders(radialProfile);
makeProviders(fittedProfile);
makeProviders(vtkField);

}
