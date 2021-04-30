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

// .NAME vtkSMMedGroupSelectionDompain - domain used to
// select groups from med files.
// .SECTION Description
// This property overrides the SetDefaultValues to select only cell groups

#ifndef __vtkSMMedGroupSelectionDomain_h
#define __vtkSMMedGroupSelectionDomain_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMSILDomain.h"

class VTK_EXPORT vtkSMMedGroupSelectionDomain : public vtkSMSILDomain
{
public:
  static vtkSMMedGroupSelectionDomain* New();
  vtkTypeMacro(vtkSMMedGroupSelectionDomain, vtkSMSILDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int SetDefaultValues(vtkSMProperty*, bool) override;

protected:
  vtkSMMedGroupSelectionDomain();
  ~vtkSMMedGroupSelectionDomain();

private:
  vtkSMMedGroupSelectionDomain(const vtkSMMedGroupSelectionDomain&);
    // Not implemented
  void operator=(const vtkSMMedGroupSelectionDomain&); // Not implemented
};

#endif
