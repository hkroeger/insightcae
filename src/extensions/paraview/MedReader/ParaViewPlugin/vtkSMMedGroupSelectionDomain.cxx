// Copyright (C) 2010-2012  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#include "vtkSMMedGroupSelectionDomain.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"

#include "vtkPVConfig.h"

#include <string>
using std::string;

vtkStandardNewMacro(vtkSMMedGroupSelectionDomain);

vtkSMMedGroupSelectionDomain::vtkSMMedGroupSelectionDomain()
{
  std::cerr<<"vtkSMMedGroupSelectionDomain::vtkSMMedGroupSelectionDomain()"<<std::endl;
}

vtkSMMedGroupSelectionDomain::~vtkSMMedGroupSelectionDomain()
{
}

int vtkSMMedGroupSelectionDomain::SetDefaultValues(vtkSMProperty* prop, bool b)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if(!svp || this->GetNumberOfRequiredProperties() == 0)
    {
    return this->Superclass::SetDefaultValues(prop, b);
    }

  // info property has default values
  vtkSMStringVectorProperty* isvp = vtkSMStringVectorProperty::SafeDownCast(
    prop->GetInformationProperty());
  if (isvp)
    {
    vtkStringList* proplist = vtkStringList::New();
    svp->SetNumberOfElements(0);
    svp->SetNumberOfElementsPerCommand(2);
    for(int id=0; id<isvp->GetNumberOfElements(); id++)
      {
      string elem = isvp->GetElement(id);
      proplist->AddString(elem.c_str());
      if(elem.find("/OnCell/") != string::npos)
        {
        proplist->AddString("1");
        }
      else
        {
        proplist->AddString("0");
        }
      }
    svp->SetElements(proplist);
    proplist->Delete();
    }

  return 1;
}

void vtkSMMedGroupSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
