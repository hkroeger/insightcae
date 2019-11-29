#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {




defineType(BoundaryCondition);
defineFactoryTable
(
    BoundaryCondition,
    LIST
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& ps
    ),
    LIST ( c, patchName, boundaryDict, ps )
);
defineStaticFunctionTable(BoundaryCondition, defaultParameters, ParameterSet);




BoundaryCondition::BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& ps)
: OpenFOAMCaseElement(c, patchName+"BC", ps),
  patchName_(patchName),
  BCtype_("UNDEFINED"),
  nFaces_(0),
  startFace_(-1)
{
    if (boundaryDict.find(patchName)!=boundaryDict.end())
    {
        BCtype_=boundaryDict.subDict(patchName).getString("type");
        nFaces_=boundaryDict.subDict(patchName).getInt("nFaces");
        startFace_=boundaryDict.subDict(patchName).getInt("startFace");
    }
}




void BoundaryCondition::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
    bndDict["type"]=BCtype_;
    bndDict["nFaces"]=nFaces_;
    bndDict["startFace"]=startFace_;
}




void BoundaryCondition::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.subDict("boundaryField");
    /*OFDictData::dict& BC=*/ boundaryField.subDict(patchName_);
  }
}




void BoundaryCondition::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (startFace_<0)
  {
      insight::Warning("BC for patch \""+patchName_+"\" provided, but patch is not present in boundaryDict!\nConfiguration of \""+patchName_+"\" is omitted.");
      return;
  }

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").insertNoDuplicate( OFDictData::data("\"libextendedFixedValueBC.so\"") );

  addIntoFieldDictionaries(dictionaries);

  OFDictData::dict bndsubd;
  addOptionsToBoundaryDict(bndsubd);
  insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
}




void BoundaryCondition::insertIntoBoundaryDict
(
  OFdicts& dictionaries,
  const std::string& patchName,
  const OFDictData::dict& bndsubd
)
{
  // contents is created as list of string / subdict pairs
  // patches have to appear ordered by "startFace"!
  OFDictData::dict& boundaryDict=dictionaries.lookupDict("constant/polyMesh/boundary");
  if (boundaryDict.size()==0)
    boundaryDict.getList("");

  OFDictData::list& bl=
    *boost::get<OFDictData::list>( &boundaryDict.begin()->second );

  //std::cout<<"Configuring "<<patchName_<<std::endl;
  // search, if patchname is already present; replace, if yes
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (std::string *name = boost::get<std::string>(&(*i)))
    {
      //cout<<"found "<<*name<<endl;
      if ( *name == patchName )
      {
        i++;
        *i=bndsubd;
        return;
      }
    }
  }

  // not found, insert (at the right location)
  OFDictData::list::iterator j = bl.end();
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (OFDictData::dict *d = boost::get<OFDictData::dict>(&(*i)))
    {
      if (d->getInt("startFace") > bndsubd.getInt("startFace") )
      {
        //std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
        j=i-1;
        break;
      }
      // patch with 0 faces has to be inserted before the face with the same start address but nonzero size
      if ( (d->getInt("startFace") == bndsubd.getInt("startFace") ) && (bndsubd.getInt("nFaces") == 0) )
      {
        //std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
        j=i-1;
        break;
      }
    }
  }
  j = bl.insert( j, OFDictData::data(patchName) );
  bl.insert( j+1, bndsubd );

  OFDictData::dict::iterator oe=boundaryDict.begin();
  std::swap( boundaryDict[boost::lexical_cast<std::string>(bl.size()/2)], oe->second );
  boundaryDict.erase(oe);
}




bool BoundaryCondition::providesBCsForPatch(const std::string& patchName) const
{
  return (patchName == patchName_);
}




bool BoundaryCondition::isPrghPressureField(const FieldList::value_type &field)
{
  return
      ( ( field.first=="pd" ) || ( field.first=="p_gh" ) || ( field.first=="p_rgh" ) )
      &&
      ( get<0> ( field.second ) == scalarField );
}


} // namespace insight
