#ifndef IQCOLLAPSIBLESECTION_H
#define IQCOLLAPSIBLESECTION_H

#include "toolkit_gui_export.h"

#include <QWidget>

class QToolButton;
class QVBoxLayout;


/**
 * @brief The IQCollapsibleSection class
 * a titled section, whose contents can be shown or hidden
 * by clicking on the title. Collapsed by default.
 */
class TOOLKIT_GUI_EXPORT IQCollapsibleSection
    : public QWidget
{
    QToolButton* toggle_;
    QWidget* contents_;

public:
    IQCollapsibleSection(
        const QString& title,
        QWidget* parent = nullptr,
        bool expanded = false );

    /**
     * @brief contentsLayout
     * layout, into which the collapsible contents shall be inserted
     */
    QVBoxLayout* contentsLayout() const;

    bool isExpanded() const;
    void setExpanded(bool expanded);
};


#endif // IQCOLLAPSIBLESECTION_H
