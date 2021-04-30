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

#ifndef _pqExtractGroupPanel_h
#define _pqExtractGroupPanel_h

#include "pqPropertyWidget.h"

#include <QTreeWidget>
class pqTreeWidgetItemObject;

class vtkSMProperty;

class pqExtractGroupPanel: public pqPropertyWidget
{
Q_OBJECT
  typedef pqPropertyWidget Superclass;
public:
  /// constructor
  pqExtractGroupPanel(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0);
  /// destructor
  ~pqExtractGroupPanel();

public slots:
  // accept changes made by this panel
  //virtual void accept();
  // reset changes made by this panel
  //virtual void reset();

protected slots:

  void updateSIL();

protected:
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  class pqUI;
  pqUI* UI;
};

#endif

