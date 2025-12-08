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
 *
 */

#include "extendedFixedValueFaPatchField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
extendedFixedValueFaPatchField<Type>::extendedFixedValueFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF
)
:
    fixedValueFaPatchField<Type>(p, iF),
    vp_()
{}


template<class Type>
extendedFixedValueFaPatchField<Type>::extendedFixedValueFaPatchField
(
    const extendedFixedValueFaPatchField<Type>& ptf,
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const faPatchFieldMapper& mapper
)
:
    fixedValueFaPatchField<Type>(ptf, p, iF, mapper),
    vp_(ptf.vp_().clone())/*,
    rescaleMode_(ptf.rescaleMode_)*/
{
#warning need automap implementation
    // vp_->autoMap(mapper);
}


template<class Type>
extendedFixedValueFaPatchField<Type>::extendedFixedValueFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFaPatchField<Type>(p, iF),
    vp_(FieldDataProvider<Type>::New(dict.lookup("source")))
{

    // if (dict.found("rescale"))
    // {
    //     word rescaleMode(dict.lookup("rescale"));
    //     if (rescaleMode=="massFlow")
    //     {
    //         auto mf=RescaleToMassFlow{
    //                 readScalar(dict.lookup("massFlow")),
    //                 dict.lookupOrDefault<word>("rhoName", "rho")
    //             };
    //         Info<<"["<<this->patch().name()<<"] Rescaling to mass flow "<<mf.massFlow<<"."<<endl;
    //         rescaleMode_=mf;
    //     }
    //     else if (rescaleMode=="volumeFlow")
    //     {
    //         auto vf = RescaleToVolumeFlow{
    //                 readScalar(dict.lookup("volumeFlow")),
    //             };
    //         Info<<"["<<this->patch().name()<<"] Rescaling to volume flow "<<vf.volumeFlow<<"."<<endl;
    //         rescaleMode_=vf;
    //     }
    //     else if (rescaleMode=="average")
    //     {
    //         auto af = RescaleToAverage{
    //             pTraits<Type>(dict.lookup("average"))
    //         };
    //         Info<<"["<<this->patch().name()<<"] Rescaling to average value "<<af.average<<"."<<endl;
    //         rescaleMode_=af;
    //     }
    //     else
    //     {
    //         FatalErrorIn("extendedFixedValueFaPatchField")
    //             <<"unrecognized rescale mode: "<<rescaleMode<<". "
    //             "Expected \"massFlow\", \"volumeFlow\" or \"average\""
    //             <<abort(FatalError);
    //     }
    // }

  if (dict.found("value"))
  {
      fixedValueFaPatchField<Type>::operator==(Field<Type>("value", dict, p.size()));
  }
  else
  {
      // Note: we use evaluate() here to trigger updateCoeffs followed
      //       by re-setting of fvatchfield::updated_ flag. This is
      //       so if first use is in the next time step it retriggers
      //       a new update.
      this->evaluate(
            #if OF_VERSION>=040000 //defined(OFdev)||defined(OFesi1806)
                      Pstream::commsTypes::blocking
            #else
                      Pstream::blocking
            #endif
                      );
  }
}


template<class Type>
extendedFixedValueFaPatchField<Type>::extendedFixedValueFaPatchField
(
    const extendedFixedValueFaPatchField<Type>& ptf
)
:
    fixedValueFaPatchField<Type>(ptf),
    vp_(ptf.vp_().clone())/*,
    rescaleMode_(ptf.rescaleMode_)*/
{}


template<class Type>
extendedFixedValueFaPatchField<Type>::extendedFixedValueFaPatchField
(
    const extendedFixedValueFaPatchField<Type>& ptf,
    const DimensionedField<Type, areaMesh>& iF
)
:
    fixedValueFaPatchField<Type>(ptf, iF),
    vp_(ptf.vp_().clone())/*,
    rescaleMode_(ptf.rescaleMode_)*/
{
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void extendedFixedValueFaPatchField<Type>::autoMap
(
    const faPatchFieldMapper& m
)
{
    fixedValueFaPatchField<Type>::autoMap(m);
#warning need automap implementation
    // vp_->autoMap(m);
}

template<class Type>
void extendedFixedValueFaPatchField<Type>::rmap
(
    const faPatchField<Type>& ptf,
    const labelList& addr
)
{
    fixedValueFaPatchField<Type>::rmap(ptf, addr);

    const extendedFixedValueFaPatchField<Type>& tiptf =
        refCast<const extendedFixedValueFaPatchField<Type> >(ptf);

    vp_->rmap(tiptf.vp_(), addr);
}


template<class Type>
void extendedFixedValueFaPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }
    fixedValueFaPatchField<Type>::operator==(
        vp_()(this->db().time().timeOutputValue(),
               this->patch().edgeCentres()) );

    Type sf=pTraits<Type>::one;

    // if (boost::get<boost::blank>(&rescaleMode_))
    // {
    //     // leave sf=1
    // }
    // else if (const auto* av = boost::get<RescaleToAverage>(&rescaleMode_))
    // {
    //     sf = cmptDivide(
    //         av->average,
    //         gSum( (*this) * (this->patch().magSf()) )/gSum(this->patch().magSf()) );

    // }
    // else
    //     FatalErrorIn("update") << "unhandled selection!" << abort(FatalError);

    // if (!boost::get<boost::blank>(&rescaleMode_))
    //     Info<<"["<<this->patch().name()<<"] rescale factor = "<<sf<<endl;

    fixedValueFaPatchField<Type>::operator==
        (cmptMultiply( sf, static_cast<faPatchField<Type>&>(*this) ));

    fixedValueFaPatchField<Type>::updateCoeffs();
}

// template<>
// void extendedFixedValueFaPatchField<vector>::updateCoeffs()
// {
//     if (this->updated())
//     {
//         return;
//     }
//     faPatchField<vector>::operator==( vp_()(this->db().time().timeOutputValue(), this->patch().Cf()) );

//     vector sf=vector::one;
//     if (boost::get<boost::blank>(&rescaleMode_))
//     {
//         // leave sf=1
//     }
//     else if (const auto* rm = boost::get<RescaleToMassFlow>(&rescaleMode_))
//     {
//         auto& rho =
//             patch().lookupPatchField<volScalarField, scalar>(
//                 rm->densityFieldName );
//         sf = (
//             rm->massFlow
//             /
//               gSum( rho * (*this) & (-this->patch().Sf()) ) ) * vector::one;
//     }
//     else if (const auto* vm = boost::get<RescaleToVolumeFlow>(&rescaleMode_))
//     {
//         sf = (
//             vm->volumeFlow
//             /
//               gSum( (*this) & (-this->patch().Sf()) ) ) * vector::one;
//     }
//     else if (const auto* av = boost::get<RescaleToAverage>(&rescaleMode_))
//     {
//         sf = cmptDivide (
//             av->average,
//             gSum( (*this) * (this->patch().magSf()) )/gSum(this->patch().magSf()) );
//     }
//     else
//         FatalErrorIn("update") << "unhandled selection!" << abort(FatalError);

//     if (!boost::get<boost::blank>(&rescaleMode_))
//         Info<<"["<<this->patch().name()<<"] Velocity rescale factor = "<<sf<<endl;

//     faPatchField<vector>::operator==
//         (cmptMultiply( sf, static_cast<faPatchField<vector>&>(*this) ));
    
//     faPatchField<vector>::updateCoeffs();
// }

template<class Type>
void extendedFixedValueFaPatchField<Type>::operator==
(
    const faPatchField<Type>& ptf
)
{
    vp_.reset(new nonuniformField<Type>(ptf));
    Field<Type>::operator=(ptf);
}


template<class Type>
void extendedFixedValueFaPatchField<Type>::operator==
(
    const Field<Type>& tf
)
{
    vp_.reset(new nonuniformField<Type>(tf));
    Field<Type>::operator=(tf);
}


template<class Type>
void extendedFixedValueFaPatchField<Type>::operator==
(
    const Type& t
)
{
    vp_.reset(new uniformField<Type>(t));
    Field<Type>::operator=(t);
}

template<class Type>
void extendedFixedValueFaPatchField<Type>::write(Ostream& os) const
{
    fixedValueFaPatchField<Type>::write(os);
    vp_().writeEntry("source", os);
    // if (boost::get<boost::blank>(&rescaleMode_))
    // {
    //     // leave sf=1
    // }
    // else if (const auto* rm = boost::get<RescaleToMassFlow>(&rescaleMode_))
    // {
    //     os << "rescale massFlow" << token::END_STATEMENT;
    //     os << "massFlow" << token::SPACE << rm->massFlow << token::END_STATEMENT;
    //     os << "rhoName" << token::SPACE << rm->densityFieldName << token::END_STATEMENT;
    // }
    // else if (const auto* vm = boost::get<RescaleToVolumeFlow>(&rescaleMode_))
    // {
    //     os << "rescale volumeFlow" << token::END_STATEMENT;
    //     os << "volumeFlow" << token::SPACE << vm->volumeFlow << token::END_STATEMENT;
    // }
    // else if (const auto* am = boost::get<RescaleToAverage>(&rescaleMode_))
    // {
    //     os << "rescale average" << token::END_STATEMENT;
    //     os << "average" << token::SPACE << am->average << token::END_STATEMENT;
    // }
    // else
    //     FatalErrorIn("write") << "unhandled selection!" << abort(FatalError);

    this->writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
