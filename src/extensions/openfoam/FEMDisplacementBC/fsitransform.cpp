/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "fsitransform.h"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{

defineTypeNameAndDebug(FSITransform, 0);
defineRunTimeSelectionTable(FSITransform, dictionary);

autoPtr<Foam::FSITransform> FSITransform::New
(
    const dictionary& dict
)
{
    if (debug)
    {
        Pout<< "coordinateSystem::New(const word&, const dictionary&) : "
            << "constructing coordinateSystem"
            << endl;
    }

    word transformType(IdentityTransform::typeName);
    
    dict.readIfPresent("transform", transformType);

    dictionaryConstructorTable::iterator cstrIter =
        dictionaryConstructorTablePtr_->find(transformType);

    if (cstrIter == dictionaryConstructorTablePtr_->end())
    {
        FatalIOErrorIn
        (
            "FSITransform::New(const dictionary&)",
            dict
        )   << "Unknown FSITransform type " << transformType << nl << nl
            << "Valid FSITransform types are :" << nl
            << dictionaryConstructorTablePtr_->toc()
            << exit(FatalIOError);
    }

    return autoPtr<FSITransform>(cstrIter()(dict));
}

FSITransform::FSITransform()
{}

FSITransform::FSITransform(const dictionary& dict)
{}

FSITransform::~FSITransform()
{}

void FSITransform::writeEntry(Ostream& os) const
{
  os << "transform" << token::SPACE << this->type() << token::END_STATEMENT << nl;
}

defineTypeNameAndDebug(IdentityTransform, 0);
addToRunTimeSelectionTable(FSITransform, IdentityTransform, dictionary);

IdentityTransform::IdentityTransform()
{}

IdentityTransform::IdentityTransform(const dictionary& dict)
: FSITransform(dict)
{}

point IdentityTransform::locationCFDtoFEM(const point& pCFD) const
{
  return pCFD;
}

vector IdentityTransform::vectorFEMtoCFD(const point& pCFD, const vector& u) const
{
  return u;
}

vector IdentityTransform::vectorCFDtoFEM(const point& pCFD, const vector& u) const
{
  return u;
}

FSITransform* IdentityTransform::clone() const
{
  return new IdentityTransform();
}

}