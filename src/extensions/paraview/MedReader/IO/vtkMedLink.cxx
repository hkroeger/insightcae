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

#include "vtkMedLink.h"

#include "vtkObjectFactory.h"

#include "vtkMedUtilities.h"

#include <string>
using namespace std;

vtkStandardNewMacro(vtkMedLink)

vtkMedLink::vtkMedLink()
{
  this->MedIterator = -1;
  this->MeshName = NULL;
  this->Link = NULL;
}

vtkMedLink::~vtkMedLink()
{
  this->SetMeshName(NULL);
  this->SetLink(NULL);
}

const char* vtkMedLink::GetFullLink(const char* originalFileName)
{
#ifdef _WIN32
  static const char sep = '\\';
#else
  static const char sep = '/';
#endif

  if(this->Link == NULL)
    {
    return NULL;
    }

  // First test if the Link is a full path, then return it.
  if(this->Link != NULL && this->Link[0] == sep)
    {
    return this->Link;
    }

  string name = string(originalFileName);
  size_t pos = name.find_last_of(sep);
  if(pos == string::npos)
    {
    return this->Link;
    }

  string clean_link = this->Link;
  string to_remove = string(".") + sep;
  int to_remove_size = to_remove.size();
  while(clean_link.substr(0, to_remove_size) == to_remove)
    clean_link = clean_link.substr(to_remove_size, string::npos);

  string path = name.substr(0, pos+1);
  this->FullLinkPath = path + clean_link;
  return this->FullLinkPath.c_str();
}

void   vtkMedLink::SetMountedIterator(med_class what, med_int mit)
{
  this->MountedIterator[what] = mit;
}

med_int  vtkMedLink::GetMountedIterator(med_class what)
{
  if(this->MountedIterator.find(what) == this->MountedIterator.end())
    return -1;

  return this->MountedIterator[what];
}

void vtkMedLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, MedIterator);
}
