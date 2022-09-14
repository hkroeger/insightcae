#ifndef IQPICKINTERACTORSTYLE_H
#define IQPICKINTERACTORSTYLE_H

#include "insightcad_gui_export.h"

#include <QObject>

#include <vtkInteractorStyleTrackballCamera.h>

class INSIGHTCAD_GUI_EXPORT IQPickInteractorStyle
      : public QObject,
        public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT

    IQPickInteractorStyle();

    int lastClickPos[2];

public:
  static IQPickInteractorStyle* New();

  vtkTypeMacro(IQPickInteractorStyle, vtkInteractorStyleTrackballCamera);

  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;

Q_SIGNALS:
  void contextMenuRequested(vtkActor* actor);

};


#endif // IQPICKINTERACTORSTYLE_H
