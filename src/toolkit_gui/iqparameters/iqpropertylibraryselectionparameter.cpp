#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

#include "iqpropertylibraryselectionparameter.h"
#include "iqparametersetmodel.h"




defineTemplateType(IQPropertyLibrarySelectionParameter);
addToFactoryTable(IQParameter, IQPropertyLibrarySelectionParameter);

addFunctionToStaticFunctionTable(
    IQParameterGridViewDelegateEditorWidget, IQPropertyLibrarySelectionParameter,
    createDelegate,
    [](QObject* parent) { return new IQSelectionDelegate(parent); }
    );
