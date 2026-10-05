#ifndef IQPARAMETERCHILDRENFORM_H
#define IQPARAMETERCHILDRENFORM_H

#include "toolkit_gui_export.h"

#include <QWidget>
#include <QPointer>

#include <set>
#include <string>
#include <vector>

#include "iqparameterbinding.h"


class QFormLayout;
class IQCADModel3DViewer;


/**
 * @brief The IQParameterChildrenForm class
 * a form, which shows one row per direct child of a parameter (set),
 * identified by its path relative to the base path of a binding group.
 *
 * Simple parameters (double, int, bool, string, vector, selections) are edited inline.
 * Nested sets and the children of selectable subsets are shown as nested forms.
 * All other parameters (sketches, paths, matrices, arrays, ...) are shown through
 * their standard edit controls (IQEmbeddedParameterEditor).
 *
 * Intended for parts of hand-built wizard forms, whose structure is not known in advance
 * (e.g. dimensions of library objects, contents of "custom" branches).
 * The form is rebuilt, whenever the set of children changes.
 */
class TOOLKIT_GUI_EXPORT IQParameterChildrenForm
    : public QWidget
{
    Q_OBJECT

    IQParameterBindingGroup* group_;
    std::string relativePath_;
    IQCADModel3DViewer* viewer_;
    std::set<std::string> excludedChildren_;
    bool showExpert_;

    QFormLayout* form_;
    std::vector<QPointer<QObject> > ownedBindings_;
    std::vector<std::pair<std::string, std::string> > signature_;

    std::vector<std::pair<std::string, std::string> > currentSignature() const;
    void clear();
    void rebuild();

public:
    IQParameterChildrenForm(
        IQParameterBindingGroup* group,
        const std::string& relativePath,
        IQCADModel3DViewer* viewer,
        const std::set<std::string>& excludedChildren = {},
        bool showExpert = false,
        QWidget* parent = nullptr );

    ~IQParameterChildrenForm();

public Q_SLOTS:
    void refresh();
};


#endif // IQPARAMETERCHILDRENFORM_H
