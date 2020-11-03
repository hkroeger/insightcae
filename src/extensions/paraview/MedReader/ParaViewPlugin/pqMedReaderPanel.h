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

#ifndef _pqMedReaderPanel_h
#define _pqMedReaderPanel_h

#include "pqPropertyWidget.h"

#include <QTreeWidget>
class pqTreeWidgetItemObject;

class vtkSMProperty;

class pqMedReaderPanel: public pqPropertyWidget
{
Q_OBJECT
  typedef pqPropertyWidget Superclass;
public:
  /// constructor
  pqMedReaderPanel(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject);
  /// destructor
  ~pqMedReaderPanel();

protected slots:
  void animationModeChanged(int mode);

  // void timeComboChanged(int timeStep);

  void updateSIL();

protected:
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  enum PixmapType
  {
    PM_NONE = -1,
    PM_POINT,
    PM_CELL,
    PM_QUADRATURE,
    PM_ELNO
  };

  void addSelectionsToTreeWidget(const QString& prop,
      QTreeWidget* tree, PixmapType pix);

  void addSelectionToTreeWidget(const QString& name,
      const QString& realName, QTreeWidget* tree, PixmapType pix,
      const QString& prop, int propIdx);

  void setupAnimationModeWidget();
  void updateAvailableTimes();

  class pqUI;
  pqUI* UI;
};

#endif
