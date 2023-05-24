#ifndef IQVTKCADMODEL3DVIEWERPICKPOINT_H
#define IQVTKCADMODEL3DVIEWERPICKPOINT_H

#include "viewwidgetaction.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerPickPoint
    : public QObject,
      public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    Q_OBJECT

public:
    IQVTKCADModel3DViewerPickPoint(
        IQVTKCADModel3DViewer &viewWidget );

    ~IQVTKCADModel3DViewerPickPoint();

    bool onLeftButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

Q_SIGNALS:
    void pickedPoint(const arma::mat& pt);
};

#endif // IQVTKCADMODEL3DVIEWERPICKPOINT_H
