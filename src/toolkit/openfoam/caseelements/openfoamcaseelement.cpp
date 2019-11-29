#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/openfoamcase.h"

namespace insight {




defineType(OpenFOAMCaseElement);
defineFactoryTable(OpenFOAMCaseElement, LIST ( OpenFOAMCase& c, const ParameterSet& ps ), LIST ( c, ps ));
defineStaticFunctionTable(OpenFOAMCaseElement, defaultParameters, ParameterSet);
defineStaticFunctionTable(OpenFOAMCaseElement, category, std::string);
defineStaticFunctionTable(OpenFOAMCaseElement, validator, ParameterSet_ValidatorPtr);
defineStaticFunctionTable(OpenFOAMCaseElement, visualizer, ParameterSet_VisualizerPtr);




int OpenFOAMCaseElement::OFversion() const
{
  return OFcase().OFversion();
}




void OpenFOAMCaseElement::modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase&, const boost::filesystem::path& ) const
{}




void OpenFOAMCaseElement::modifyMeshOnDisk(const OpenFOAMCase&, const boost::filesystem::path&) const
{}




void OpenFOAMCaseElement::modifyCaseOnDisk(const OpenFOAMCase&, const boost::filesystem::path&) const
{}




void OpenFOAMCaseElement::addFields( OpenFOAMCase& ) const
{}




OpenFOAMCaseElement::OpenFOAMCaseElement(OpenFOAMCase& c, const std::string& name, const ParameterSet& ps)
: CaseElement(c, name, ps)
{}




bool OpenFOAMCaseElement::providesBCsForPatch(const std::string&) const
{
  return false;
}




std::string OpenFOAMCaseElement::category()
{ return "Uncategorized"; }




ParameterSet_ValidatorPtr OpenFOAMCaseElement::validator()
{
  return ParameterSet_ValidatorPtr();
}




ParameterSet_VisualizerPtr OpenFOAMCaseElement::visualizer()
{
  return ParameterSet_VisualizerPtr();
}




bool OpenFOAMCaseElement::isInConflict(const CaseElement&)
{
  return false;
}




const OpenFOAMCase& OpenFOAMCaseElement::OFcase() const
{
  return *static_cast<OpenFOAMCase*>(&case_);
}




OpenFOAMCase& OpenFOAMCaseElement::OFcase()
{
  return *static_cast<OpenFOAMCase*>(&case_);
}




} // namespace insight
