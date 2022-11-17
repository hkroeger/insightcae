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
#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/openfoamcase.h"

namespace insight {




defineType(OpenFOAMCaseElement);
defineFactoryTable(OpenFOAMCaseElement, LIST ( OpenFOAMCase& c, const ParameterSet& ps ), LIST ( c, ps ));
defineStaticFunctionTable(OpenFOAMCaseElement, defaultParameters, ParameterSet);
defineStaticFunctionTable(OpenFOAMCaseElement, category, std::string);
defineStaticFunctionTable(OpenFOAMCaseElement, validator, ParameterSet_ValidatorPtr);
defineStaticFunctionTable(OpenFOAMCaseElement, visualizer, std::shared_ptr<ParameterSetVisualizer>);




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




std::shared_ptr<ParameterSetVisualizer> OpenFOAMCaseElement::visualizer()
{
  return std::shared_ptr<ParameterSetVisualizer>();
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
