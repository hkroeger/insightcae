#include "iqcollapsiblesection.h"

#include <QToolButton>
#include <QVBoxLayout>




IQCollapsibleSection::IQCollapsibleSection(
    const QString &title,
    QWidget *parent,
    bool expanded )
    : QWidget(parent)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0,0,0,0);

    toggle_ = new QToolButton(this);
    toggle_->setText(title);
    toggle_->setCheckable(true);
    toggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle_->setAutoRaise(true);
    l->addWidget(toggle_);

    contents_ = new QWidget(this);
    auto *cl = new QVBoxLayout(contents_);
    cl->setContentsMargins(0,0,0,0);
    l->addWidget(contents_);

    connect(toggle_, &QToolButton::toggled,
            this, &IQCollapsibleSection::setExpanded);

    setExpanded(expanded);
}




QVBoxLayout *IQCollapsibleSection::contentsLayout() const
{
    return static_cast<QVBoxLayout*>(contents_->layout());
}




bool IQCollapsibleSection::isExpanded() const
{
    return toggle_->isChecked();
}




void IQCollapsibleSection::setExpanded(bool expanded)
{
    if (toggle_->isChecked()!=expanded)
        toggle_->setChecked(expanded); // re-enters via toggled()

    toggle_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    contents_->setVisible(expanded);
}
