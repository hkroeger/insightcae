#ifndef IQPARAMETERBINDING_H
#define IQPARAMETERBINDING_H

#include "toolkit_gui_export.h"

#include <QObject>
#include <QPointer>
#include <QWidget>

#include <functional>
#include <string>
#include <vector>

#include "iqparametersetmodelobserver.h"


class QLineEdit;
class QCheckBox;
class QComboBox;
class IQParameterBindingGroup;




/**
 * @brief The IQParameterBinding class
 * Connects a widget to a single parameter, which is identified by its path
 * relative to the base path of the owning binding group.
 *
 * Widget edits are written directly into the parameter, which then notifies
 * the parameter set model (and through it all other views and the visualization).
 * Changes of the parameter from elsewhere are read back on refresh().
 *
 * If the parameter path cannot be resolved, the widget is disabled
 * (or hidden, see setHideWhenUnresolved).
 */
class TOOLKIT_GUI_EXPORT IQParameterBinding
    : public QObject
{
    Q_OBJECT

    IQParameterBindingGroup* group_;
    std::string relativePath_;
    QPointer<QWidget> widget_;
    std::vector<QPointer<QWidget> > companions_;
    bool hideWhenUnresolved_ = false;

protected:
    /**
     * @brief readFromParameter
     * update the widget from the parameter value.
     * Called with the widget's signals blocked.
     */
    virtual void readFromParameter(const insight::Parameter& p) =0;

    /**
     * @brief write
     * write into the parameter using the supplied function
     */
    void write(std::function<void(insight::Parameter&)> f);

    template<class P>
    void writeAs(std::function<void(P&)> f)
    {
        write([&f](insight::Parameter& p) { f(dynamic_cast<P&>(p)); });
    }

    /**
     * @brief userIsEditing
     * true, if the widget has the focus and contains edits,
     * which have not been written yet. Then, refresh() will not touch the widget.
     */
    virtual bool userIsEditing() const;

public:
    IQParameterBinding(
        IQParameterBindingGroup* group,
        const std::string& relativePath,
        QWidget* widget );

    IQParameterBindingGroup* group() const;
    const std::string& relativePath() const;
    std::string fullPath() const;
    QWidget* widget() const;

    /**
     * @brief alsoAffect
     * widgets (e.g. labels), which shall be enabled/disabled or shown/hidden along with the bound widget
     */
    IQParameterBinding* alsoAffect(QWidget* w);
    IQParameterBinding* setHideWhenUnresolved(bool hide=true);

    void refresh();
};




class TOOLKIT_GUI_EXPORT IQDoubleBinding : public IQParameterBinding
{
    Q_OBJECT
protected:
    void readFromParameter(const insight::Parameter& p) override;
    bool userIsEditing() const override;
public:
    IQDoubleBinding(IQParameterBindingGroup* group, const std::string& path, QLineEdit* le);
};


class TOOLKIT_GUI_EXPORT IQIntBinding : public IQParameterBinding
{
    Q_OBJECT
protected:
    void readFromParameter(const insight::Parameter& p) override;
    bool userIsEditing() const override;
public:
    IQIntBinding(IQParameterBindingGroup* group, const std::string& path, QLineEdit* le);
};


class TOOLKIT_GUI_EXPORT IQStringBinding : public IQParameterBinding
{
    Q_OBJECT
protected:
    void readFromParameter(const insight::Parameter& p) override;
    bool userIsEditing() const override;
public:
    IQStringBinding(IQParameterBindingGroup* group, const std::string& path, QLineEdit* le);
};


class TOOLKIT_GUI_EXPORT IQBoolBinding : public IQParameterBinding
{
    Q_OBJECT
protected:
    void readFromParameter(const insight::Parameter& p) override;
public:
    IQBoolBinding(IQParameterBindingGroup* group, const std::string& path, QCheckBox* cb);
};


/**
 * @brief The IQVectorBinding class
 * binds a vector parameter to a widget with one line edit per component
 */
class TOOLKIT_GUI_EXPORT IQVectorBinding : public IQParameterBinding
{
    Q_OBJECT
    std::vector<QLineEdit*> components_;
protected:
    void readFromParameter(const insight::Parameter& p) override;
    bool userIsEditing() const override;
public:
    IQVectorBinding(IQParameterBindingGroup* group, const std::string& path, QWidget* parent=nullptr);
};


/**
 * @brief The IQSelectionBinding class
 * binds any parameter implementing SelectionParameterInterface
 * (selection, selectable subset, library selection) to a combo box
 */
class TOOLKIT_GUI_EXPORT IQSelectionBinding : public IQParameterBinding
{
    Q_OBJECT
    std::function<QString(const std::string&)> keyLabel_;
protected:
    void readFromParameter(const insight::Parameter& p) override;
public:
    IQSelectionBinding(
        IQParameterBindingGroup* group, const std::string& path, QComboBox* cb,
        std::function<QString(const std::string&)> keyLabel = nullptr );
};




/**
 * @brief The IQParameterBindingGroup class
 * owns a set of bindings, which share a common base path.
 * Watches the parameter set model and refreshes all bindings on change.
 *
 * The base path can be changed at any time (e.g. when a different array element is selected),
 * which rebinds all widgets of a form to the newly selected element.
 */
class TOOLKIT_GUI_EXPORT IQParameterBindingGroup
    : public QObject
{
    Q_OBJECT

    IQParameterSetModel* model_;
    IQParameterSetModelObserver* observer_;
    std::string basePath_;
    bool active_;

    std::vector<QPointer<IQParameterBinding> > bindings_;

    struct VisibilityRule
    {
        QPointer<QWidget> widget;
        std::function<bool()> isVisible;
    };
    std::vector<VisibilityRule> visibilityRules_;

public:
    IQParameterBindingGroup(
        IQParameterSetModel* model,
        QObject* parent,
        const std::string& basePath = std::string() );

    IQParameterSetModel* model() const;
    IQParameterSetModelObserver* observer() const;

    const std::string& basePath() const;
    std::string fullPath(const std::string& relativePath) const;

    /**
     * @brief isActive
     * false, if no base path is set (e.g. no array element selected).
     * Then, all bound widgets are disabled.
     */
    bool isActive() const;

    void setBasePath(const std::string& basePath);
    void clearBasePath();

    bool hasParameter(const std::string& relativePath) const;
    const insight::Parameter* parameter(const std::string& relativePath) const;

    template<class P>
    const P* parameterAs(const std::string& relativePath) const
    {
        return dynamic_cast<const P*>(parameter(relativePath));
    }

    /**
     * @brief modify
     * apply a modification to the parameter at relativePath
     */
    bool modify(
        const std::string& relativePath,
        std::function<void(insight::Parameter&)> modification,
        QWidget* messageParent = nullptr );

    /**
     * @brief selectionKey
     * current key of a selection(-like) parameter, empty, if not resolvable
     */
    std::string selectionKey(const std::string& relativePath) const;

    void addBinding(IQParameterBinding* b);

    void addVisibilityRule(QWidget* w, std::function<bool()> isVisible);
    void showIfPathExists(QWidget* w, const std::string& relativePath);
    void showIfSelected(QWidget* w, const std::string& relativePath, const std::string& key);

    // convenience factories, returning the (created) widget
    QLineEdit* bindDouble(const std::string& relativePath, QLineEdit* le=nullptr);
    QLineEdit* bindInt(const std::string& relativePath, QLineEdit* le=nullptr);
    QLineEdit* bindString(const std::string& relativePath, QLineEdit* le=nullptr);
    QCheckBox* bindBool(const std::string& relativePath, QCheckBox* cb=nullptr);
    QComboBox* bindSelection(
        const std::string& relativePath, QComboBox* cb=nullptr,
        std::function<QString(const std::string&)> keyLabel = nullptr );
    QWidget* bindVector(const std::string& relativePath);

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    /**
     * @brief refreshed
     * emitted after all bindings have been updated
     */
    void refreshed();
    void basePathChanged(const std::string& basePath);
};


#endif // IQPARAMETERBINDING_H
