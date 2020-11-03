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

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCylinderSource.h"
#include "vtkMedReader.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkPolyDataNormals.h"
#include "vtkMedReader.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int main(int argc, char *argv[])
{
  if (!argv[1])
  {
    std::cerr<<"a file name is expected as first command line argument!"<<std::endl;
    return -1;
  }

  vtkMedReader* reader = vtkMedReader::New();
  reader->SetFileName(argv[1]);
  reader->Update();

  vtkCompositePolyDataMapper *mapper = vtkCompositePolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer *renderer = vtkRenderer::New();
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400,400);
  renWin->Render();
  interactor->Initialize();
  renderer->ResetCamera();
  renWin->Render();
  renderer->ResetCamera();

  int retVal = vtkRegressionTestImageThreshold(renWin,18);
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    interactor->Start();
    }

  reader->Delete();
  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
