#ifndef IQEMBEDDEDPARAMETEREDITOR_H
#define IQEMBEDDEDPARAMETEREDITOR_H

#include "toolkit_gui_export.h"

#include <QWidget>
#include <QPointer>

#include "iqparameterbinding.h"


class IQParameter;
class IQCADModel3DViewer;


/**
 * @brief The IQEmbeddedParameterEditor class
 * displays the standard edit controls (as shown below the parameter tree)
 * of a single parameter, identified by its path relative to the base path of a binding group.
 *
 * Intended for parameters, for which a dedicated form makes no sense,
 * e.g. CAD sketches, transformations or complex nested sets.
 *
 * The controls are recreated, whenever the parameter at the path is replaced
 * (base path change, model reset, removal).
 * The widget hides itself, if the path cannot be resolved.
 */
class TOOLKIT_GUI_EXPORT IQEmbeddedParameterEditor
    : public QWidget
{
    Q_OBJECT

    IQParameterBindingGroup* group_;
    std::string relativePath_;
    IQCADModel3DViewer* viewer_;

    QPointer<IQParameter> currentParameter_;
    std::string currentPath_;
    QWidget* contents_;

    void rebuild(IQParameter* p);

public:
    IQEmbeddedParameterEditor(
        IQParameterBindingGroup* group,
        const std::string& relativePath,
        IQCADModel3DViewer* viewer,
        QWidget* parent = nullptr );

public Q_SLOTS:
    void refresh();
};


#endif // IQEMBEDDEDPARAMETEREDITOR_H
