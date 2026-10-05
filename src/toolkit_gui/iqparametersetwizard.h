#ifndef IQPARAMETERSETWIZARD_H
#define IQPARAMETERSETWIZARD_H

#include "toolkit_gui_export.h"

#include <QWidget>

#include <map>
#include <string>

#include "iqparameterbinding.h"
#include "iqembeddedparametereditor.h"
#include "iqarrayelementselector.h"


class QTabWidget;
class QPushButton;
class QFormLayout;
class QLineEdit;
class QCheckBox;
class QComboBox;
class IQCADModel3DViewer;
class IQParameterSetWizard;




/**
 * @brief The IQParameterSetWizardPage class
 * base class for a single page of a parameter set wizard.
 * Provides access to the model, the 3D viewer and a binding group (with empty base path)
 * plus helpers for creating synchronized form elements.
 */
class TOOLKIT_GUI_EXPORT IQParameterSetWizardPage
    : public QWidget
{
    Q_OBJECT

    IQParameterSetWizard* wizard_;
    IQParameterBindingGroup* bindings_;

public:
    IQParameterSetWizardPage(IQParameterSetWizard* wizard);

    IQParameterSetWizard* wizard() const;
    IQParameterSetModel* model() const;
    IQCADModel3DViewer* viewer() const;

    /**
     * @brief bindings
     * binding group, which resolves paths relative to the root of the parameter set
     */
    IQParameterBindingGroup* bindings() const;

    /**
     * @brief createBindingGroup
     * create an additional binding group, e.g. for a detail form
     * which follows the selection of an array element selector
     */
    IQParameterBindingGroup* createBindingGroup();

    /**
     * @brief createEmbeddedEditor
     * embed the standard edit controls of a parameter
     */
    IQEmbeddedParameterEditor* createEmbeddedEditor(
        IQParameterBindingGroup* group,
        const std::string& relativePath );

    /**
     * @brief createContextSelector
     * create an element selector, whose selection is shared via the named wizard context
     * with all other selectors of the same context (on all pages)
     */
    IQArrayElementSelector* createContextSelector(
        const std::string& contextName,
        IQParameterBindingGroup* group,
        const std::string& relativeArrayPath,
        IQArrayElementSelector::Mode mode = IQArrayElementSelector::ComboBox,
        int buttons = IQArrayElementSelector::NoButtons );

    /**
     * @brief createDetailGroup
     * create a binding group, whose base path follows the
     * element selected in the given selector (inactive, if nothing is selected)
     */
    IQParameterBindingGroup* createDetailGroup(IQArrayElementSelector* selector);

    /**
     * @brief humanize
     * convert a selection key or parameter name into a label
     * ("fromLibrary" => "From library")
     */
    static QString humanize(const std::string& key);

    // Form rows, bound to a parameter. The row (incl. label) is hidden,
    // if the parameter path cannot be resolved (e.g. inactive subset branch).
    static QLineEdit* addDoubleRow(
        QFormLayout* form, IQParameterBindingGroup* group,
        const QString& label, const std::string& relativePath );
    static QLineEdit* addIntRow(
        QFormLayout* form, IQParameterBindingGroup* group,
        const QString& label, const std::string& relativePath );
    static QLineEdit* addStringRow(
        QFormLayout* form, IQParameterBindingGroup* group,
        const QString& label, const std::string& relativePath );
    static QCheckBox* addBoolRow(
        QFormLayout* form, IQParameterBindingGroup* group,
        const QString& label, const std::string& relativePath );
    static QWidget* addVectorRow(
        QFormLayout* form, IQParameterBindingGroup* group,
        const QString& label, const std::string& relativePath );
    /**
     * @brief addSelectionRow
     * combo box for a selection, selectable subset or library selection.
     * If no keyLabel function is given, the keys are humanized.
     */
    static QComboBox* addSelectionRow(
        QFormLayout* form, IQParameterBindingGroup* group,
        const QString& label, const std::string& relativePath,
        std::function<QString(const std::string&)> keyLabel = nullptr );

    /**
     * @brief addBindingRow
     * add a row for an arbitrary binding
     */
    static void addBindingRow(
        QFormLayout* form, const QString& label, IQParameterBinding* binding );
};




/**
 * @brief The IQParameterSetWizard class
 * base class for input wizards: a tabbed set of forms,
 * each showing an organized subset of a parameter set.
 *
 * All edits are made directly in the parameter set of the supplied model,
 * so that the parameter tree and the 3D visualization follow automatically.
 * All pages follow changes made elsewhere through their binding groups.
 *
 * Additionally, the wizard holds named context values (e.g. the currently selected storey),
 * which can be shared between pages.
 */
class TOOLKIT_GUI_EXPORT IQParameterSetWizard
    : public QWidget
{
    Q_OBJECT

    IQParameterSetModel* model_;
    IQCADModel3DViewer* viewer_;
    QTabWidget* tabs_;
    QPushButton *prevBtn_, *nextBtn_;

    std::map<std::string, std::string> context_;

    void updateNavigationButtons();

public:
    IQParameterSetWizard(
        IQParameterSetModel* model,
        IQCADModel3DViewer* viewer,
        const QString& title,
        QWidget* parent = nullptr );

    IQParameterSetModel* model() const;
    IQCADModel3DViewer* viewer() const;

    /**
     * @brief addPage
     * add a page, the wizard takes ownership
     */
    void addPage(IQParameterSetWizardPage* page, const QString& title);

    /**
     * @brief addTab
     * add an arbitrary widget as a tab (e.g. the complete parameter tree),
     * the wizard takes ownership
     */
    void addTab(QWidget* widget, const QString& title);

    std::string contextValue(const std::string& name) const;

public Q_SLOTS:
    void setContextValue(const std::string& name, const std::string& value);

Q_SIGNALS:
    void contextChanged(const std::string& name, const std::string& value);
};




/**
 * @brief newWizard
 * helper for registration in CADParameterSetModelVisualizer::createGUIWizardForAnalysis
 */
template<class W>
QWidget* newWizard(IQParameterSetModel* pm, IQCADModel3DViewer* viewer)
{
    return new W(pm, viewer);
}


#endif // IQPARAMETERSETWIZARD_H
