#ifndef IQARRAYELEMENTSELECTOR_H
#define IQARRAYELEMENTSELECTOR_H

#include "toolkit_gui_export.h"

#include <QWidget>

#include <functional>
#include <string>
#include <vector>

#include "iqparameterbinding.h"


class QComboBox;
class QListWidget;
class QPushButton;


/**
 * @brief The IQArrayElementSelector class
 * lists the elements of an array or labeled array parameter
 * (path relative to the base path of a binding group)
 * and lets the user pick one. Optionally offers buttons for
 * adding, removing, duplicating and renaming elements.
 *
 * The list follows changes of the parameter set made elsewhere.
 * The selection is retained by key (labeled array) or by row (array).
 * The full path of the selected element is reported through currentElementChanged,
 * typically connected to IQParameterBindingGroup::setBasePath of a detail form.
 */
class TOOLKIT_GUI_EXPORT IQArrayElementSelector
    : public QWidget
{
    Q_OBJECT

public:
    enum Mode { ComboBox, List };

    enum Button {
        NoButtons = 0,
        Add = 1,
        Remove = 2,
        Duplicate = 4,
        Rename = 8,
        AllButtons = Add|Remove|Duplicate|Rename
    };

    typedef std::function<QString(
        const insight::Parameter& element,
        int index,
        const std::string& key )> LabelFunction;

private:
    IQParameterBindingGroup* group_;
    std::string relativeArrayPath_;
    Mode mode_;

    QComboBox* combo_ = nullptr;
    QListWidget* list_ = nullptr;
    QPushButton *addBtn_=nullptr, *removeBtn_=nullptr,
                *duplicateBtn_=nullptr, *renameBtn_=nullptr;

    LabelFunction labelFunction_;

    std::vector<std::string> keys_;
    std::string emittedPath_;
    bool emittedValid_ = false;

    const insight::Parameter* arrayParameter() const;
    bool isLabeled() const;
    bool keysLocked() const;

    int viewRow() const;
    void setViewRow(int r);
    void setItems(const QStringList& labels);

    void selectKey(const std::string& key);
    void emitIfChanged();

    void addElement();
    void removeElement();
    void duplicateElement();
    void renameElement();

public:
    IQArrayElementSelector(
        IQParameterBindingGroup* group,
        const std::string& relativeArrayPath,
        Mode mode = ComboBox,
        int buttons = NoButtons,
        QWidget* parent = nullptr );

    void setLabelFunction(LabelFunction lf);

    int currentRow() const;
    std::string currentKey() const;

    /**
     * @brief currentElementPath
     * full path of the selected element, empty if none is selected
     */
    std::string currentElementPath() const;

    void setCurrentRow(int row);

    /**
     * @brief setCurrentElementPath
     * select the element with the given full path, if it belongs to this array
     */
    void setCurrentElementPath(const std::string& path);

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void currentElementChanged(const std::string& elementPath);
};


#endif // IQARRAYELEMENTSELECTOR_H
