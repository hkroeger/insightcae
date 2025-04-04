
#include "inventornavigationmanager.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkcadmodel3dviewerpanning.h"
#include "iqvtkcadmodel3dviewerrotation.h"
#include "base/factory.h"

typedef InventorNavigationManager<
    IQVTKCADModel3DViewer,
    IQVTKCADModel3DViewerPanning,
    IQVTKCADModel3DViewerRotation >
    VTKInventorNavigation;

defineTemplateType(VTKInventorNavigation);

addToFactoryTable2(
    VTKNavigationManager,
    NavigationManagerFactory, navigationManagers,
    VTKInventorNavigation
    );
