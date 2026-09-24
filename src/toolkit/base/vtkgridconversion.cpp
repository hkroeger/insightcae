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


#include "vtkgridconversion.h"

#include "base/exception.h"

#include "vtkDataSet.h"
#include "vtkAppendFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFieldData.h"

namespace insight
{

vtkSmartPointer<vtkUnstructuredGrid>
multiBlockDataSetToUnstructuredGrid(vtkDataObject *input)
{
    auto output = vtkSmartPointer<vtkUnstructuredGrid>::New();

    bool MergePoints = true;
    vtkIdType SubTreeCompositeIndex = 0;
    double Tolerance = 0.;

    auto AddDataSet = [](vtkDataSet* ds, vtkAppendFilter* appender)
    {
      vtkDataSet* clone = ds->NewInstance();
      clone->ShallowCopy(ds);
      appender->AddInputData(clone);
      clone->Delete();
    };

    auto ExecuteSubTree = [](
      vtkCompositeDataSet* curCD, vtkAppendFilter* appender)
    {
      vtkCompositeDataIterator* iter2 = curCD->NewIterator();
      for (iter2->InitTraversal(); !iter2->IsDoneWithTraversal(); iter2->GoToNextItem())
      {
        vtkDataSet* curDS = vtkDataSet::SafeDownCast(iter2->GetCurrentDataObject());
        if (curDS)
        {
          appender->AddInputData(curDS);
        }
      }
      iter2->Delete();
    };



    vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(input);
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);

    if (ug)
    {
      output->ShallowCopy(ug);
    }
    else
    {
      auto appender = vtkSmartPointer<vtkAppendFilter>::New();
      appender->SetMergePoints(MergePoints ? 1 : 0);
      if (MergePoints)
      {
#if (VTK_MAJOR_VERSION>8 || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=90) )
        appender->SetTolerance(Tolerance);
#endif
      }
      if (ds)
      {
        AddDataSet(ds, appender);
      }
      else if (cd)
      {
        if (SubTreeCompositeIndex == 0)
        {
          ExecuteSubTree(cd, appender);
        }
        vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(cd->NewIterator());
        if (!iter)
        {
          throw insight::Exception("Composite data is not a tree");
        }
        iter->VisitOnlyLeavesOff();
        for (iter->InitTraversal();
             !iter->IsDoneWithTraversal() && iter->GetCurrentFlatIndex() <= SubTreeCompositeIndex;
             iter->GoToNextItem())
        {
          if (iter->GetCurrentFlatIndex() == SubTreeCompositeIndex)
          {
            vtkDataObject* curDO = iter->GetCurrentDataObject();
            vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
            vtkUnstructuredGrid* curUG = vtkUnstructuredGrid::SafeDownCast(curDO);
            vtkDataSet* curDS = vtkUnstructuredGrid::SafeDownCast(curDO);
            if (curUG)
            {
              output->ShallowCopy(curUG);
              // NOTE: Not using the appender at all.
            }
            else if (curDS && curCD->GetNumberOfPoints() > 0)
            {
              AddDataSet(curDS, appender);
            }
            else if (curCD)
            {
              ExecuteSubTree(curCD, appender);
            }
            break;
          }
        }
        iter->Delete();
      }

      if (appender->GetNumberOfInputConnections(0) > 0)
      {
        appender->Update();
        output->ShallowCopy(appender->GetOutput());
        // this will override field data the vtkAppendFilter passed from the first
        // block. It seems like a reasonable approach, if global field data is
        // present.
        if (ds)
        {
          output->GetFieldData()->PassData(ds->GetFieldData());
        }
        else if (cd)
        {
          output->GetFieldData()->PassData(cd->GetFieldData());
        }

      }
//          RemovePartialArrays(output);
    }

    return output;
}

}
