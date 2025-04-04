#include "touchpadnavigationmanager.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkcadmodel3dviewerpanning.h"
#include "iqvtkcadmodel3dviewerrotation.h"

#include "base/factory.h"

typedef TouchpadNavigationManager<
    IQVTKCADModel3DViewer,
    IQVTKCADModel3DViewerPanning,
    IQVTKCADModel3DViewerRotation >
    VTKTouchpadNavigation;

defineTemplateType(VTKTouchpadNavigation);

addToFactoryTable2(
    VTKNavigationManager,
    NavigationManagerFactory, navigationManagers,
    VTKTouchpadNavigation
    );

