#include "simplebc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {


defineType(SimpleBC);
addToFactoryTable(BoundaryCondition, SimpleBC);
addToStaticFunctionTable(BoundaryCondition, SimpleBC, defaultParameters);

void SimpleBC::init()
{
    BCtype_ = p_.className;
    if ( ( OFversion() >=230 ) && ( BCtype_=="symmetryPlane" ) ) {
        BCtype_="symmetry";
    }
}

SimpleBC::SimpleBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className )
    : BoundaryCondition ( c, patchName, boundaryDict, Parameters().set_className(className) ),
    p_(Parameters().set_className(className))
{
    init();
}

SimpleBC::SimpleBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p )
    : BoundaryCondition ( c, patchName, boundaryDict, p ),
      p_ ( p )
{
    init();
}

void SimpleBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {

        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );

        if ( ( BCtype_=="cyclic" ) && ( ( field.first=="motionU" ) || ( field.first=="pointDisplacement" ) ) ) {
            MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC );
        } else {
            std::string tname=BCtype_;
            BC["type"]=OFDictData::data ( tname );
        }

    }
}


} // namespace insight
