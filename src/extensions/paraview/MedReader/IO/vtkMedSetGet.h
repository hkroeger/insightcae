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

#ifndef _vtkMedSetGet_h_
#define _vtkMedSetGet_h_

//BTX
template <class T> class vtkObjectVector;
template <class T> class vtkList;
template <class T> class vtkMedComputeStepMap;

#define vtkGetObjectVectorMacro(name, type)\
  virtual type* Get##name (int index);\
  virtual vtkIdType GetNumberOf##name ();

#define vtkSetObjectVectorMacro(name, type)\
  virtual void  AllocateNumberOf##name (vtkIdType size);\
  virtual void  Set##name (vtkIdType index, type* obj);\
  virtual void  Append##name (type* obj);\
  virtual void  Remove##name (type* obj);

#define vtkSetAbstractObjectVectorMacro(name, type)\
  virtual void SetNumberOf##name (vtkIdType size);\
  virtual void Set##name (vtkIdType index, type* obj);\
  virtual void Append##name (type* obj);

#define vtkCxxGetObjectVectorMacro(class, name, type)\
  type* class::Get##name (int index)\
  {\
    if(index < 0 || index >= this->name->size())\
      return NULL;\
    return this->name->at(index);\
  }\
  vtkIdType class::GetNumberOf##name ()\
  {\
    return this->name->size();\
  }

#define vtkCxxSetObjectVectorMacro(class, name, type)\
  void  class::AllocateNumberOf##name (vtkIdType size)\
  {\
    if(this->name->size() == size)\
      return;\
    if(size <= 0 )\
      this->name->clear();\
    else\
      this->name->resize(size);\
    for(vtkIdType _ii=0; _ii<this->name->size(); _ii++)\
      {\
        this->name->at(_ii) = vtkSmartPointer< type > ::New();\
      }\
    this->Modified();\
  }\
  void class::Set##name (vtkIdType index, type* obj)\
  {\
    if(index < 0 || index >= this->name->size())\
    {\
      vtkWarningMacro("has not been allocated before setting value" );\
      return;\
    }\
    if( this->name->at(index) == obj)\
      return;\
    this->name->at(index) = obj; \
    this->Modified();\
  }\
  void  class::Append##name (type* obj)\
  {\
    this->name->resize(this->name->size()+1);\
    this->name->at(this->name->size()-1) = obj;\
    this->Modified();\
  }\
  void  class::Remove##name (type* obj)\
  {\
    vtkIdType index=0;\
    for(index=0; index < this->name->size(); index++)\
     {\
       if(this->name->at(index) == obj) break;\
     }\
    if(index == this->name->size()) return;\
    for(vtkIdType id=index; id < this->name->size()-1; id++)\
      {\
        this->name->at(id) = this->name->at(id+1);\
      }\
    this->name->resize(this->name->size()-1);\
    this->Modified();\
  }

#define vtkCxxSetAbstractObjectVectorMacro(class, name, type)\
  void  class::SetNumberOf##name (vtkIdType size)\
  {\
    if(this->name->size() == size)\
      return;\
    if(size <= 0 )\
      this->name->clear();\
    else\
      this->name->resize(size);\
    this->Modified();\
  }\
  void class::Set##name (vtkIdType index, type* obj)\
  {\
    if(index < 0 || index >= this->name->size())\
    {\
      vtkWarningMacro("has not been allocated before setting value" );\
      return;\
    }\
    if( this->name->at(index) == obj)\
      return;\
    this->name->at(index) = obj; \
    this->Modified();\
  }\
  void  class::Append##name (type* obj)\
  {\
    this->name->resize(this->name->size()+1);\
    this->name->at(this->name->size()-1) = obj;\
    this->Modified();\
  }

//ETX

#endif
