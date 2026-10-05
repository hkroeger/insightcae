#include "iqparameterchildrenform.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>

#include "base/parameters/simpleparameter.h"
#include "base/parameters/selectionparameter.h"
#include "base/parameters/subsetparameter.h"

#include "iqembeddedparametereditor.h"




IQParameterChildrenForm::IQParameterChildrenForm(
    IQParameterBindingGroup *group,
    const std::string &relativePath,
    IQCADModel3DViewer *viewer,
    const std::set<std::string> &excludedChildren,
    bool showExpert,
    QWidget *parent )
    : QWidget(parent),
      group_(group),
      relativePath_(relativePath),
      viewer_(viewer),
      excludedChildren_(excludedChildren),
      showExpert_(showExpert)
{
    form_ = new QFormLayout(this);
    form_->setContentsMargins(0,0,0,0);

    connect(group_, &IQParameterBindingGroup::refreshed,
            this, &IQParameterChildrenForm::refresh);

    group_->observer()->scheduleNotification();
}




IQParameterChildrenForm::~IQParameterChildrenForm()
{
    clear();
}




std::vector<std::pair<std::string, std::string> >
IQParameterChildrenForm::currentSignature() const
{
    std::vector<std::pair<std::string, std::string> > sig;
    if (auto *p = group_->parameter(relativePath_))
    {
        for (int i=0; i<p->nChildren(); ++i)
        {
            auto name = p->childElementName(i);
            if (name.empty() || name[0]=='<' || excludedChildren_.count(name))
                continue; // skip pseudo-children of selectable subsets

            auto *cp = dynamic_cast<const insight::Parameter*>(&p->childElement(i));
            if (!cp || cp->isHidden() || (cp->isExpert() && !showExpert_))
                continue;

            sig.push_back({name, cp->type()});
        }
    }
    return sig;
}




void IQParameterChildrenForm::clear()
{
    for (auto& b: ownedBindings_)
    {
        if (b) delete b.data();
    }
    ownedBindings_.clear();

    while (form_->rowCount()>0)
    {
        form_->removeRow(0);
    }
}




void IQParameterChildrenForm::rebuild()
{
    clear();

    auto *p = group_->parameter(relativePath_);
    if (!p) return;

    auto addBinding = [this](IQParameterBinding* b)
    {
        ownedBindings_.push_back(b);
        group_->addBinding(b);
        return b->widget();
    };

    for (const auto& s: signature_)
    {
        const auto& name = s.first;
        auto rel = joinParameterPath(relativePath_, name);
        auto& cp = dynamic_cast<const insight::Parameter&>(
            p->childElementByName(name) );

        auto label = QString::fromStdString(name);
        auto tip = QString::fromStdString(cp.description().toPlainText());

        QWidget* field = nullptr;
        QWidget* nested = nullptr;

        if (dynamic_cast<const insight::SelectionParameterInterface*>(&cp))
        {
            field = addBinding(new IQSelectionBinding(group_, rel, new QComboBox));
            if (cp.nChildren()>0)
            {
                // selectable subset or library selection with parameters
                nested = new IQParameterChildrenForm(
                    group_, rel, viewer_, {}, showExpert_ );
            }
        }
        else if (dynamic_cast<const insight::VectorParameter*>(&cp))
            field = addBinding(new IQVectorBinding(group_, rel));
        else if (dynamic_cast<const insight::DoubleParameter*>(&cp))
            field = addBinding(new IQDoubleBinding(group_, rel, new QLineEdit));
        else if (dynamic_cast<const insight::IntParameter*>(&cp))
            field = addBinding(new IQIntBinding(group_, rel, new QLineEdit));
        else if (dynamic_cast<const insight::BoolParameter*>(&cp))
            field = addBinding(new IQBoolBinding(group_, rel, new QCheckBox));
        else if (dynamic_cast<const insight::StringParameter*>(&cp)) // selections are handled above
            field = addBinding(new IQStringBinding(group_, rel, new QLineEdit));
        else if (dynamic_cast<const insight::ParameterSet*>(&cp))
        {
            auto *gb = new QGroupBox(label);
            gb->setToolTip(tip);
            auto *l = new QVBoxLayout(gb);
            l->addWidget(new IQParameterChildrenForm(
                group_, rel, viewer_, {}, showExpert_ ));
            form_->addRow(gb);
            continue;
        }
        else
        {
            nested = new IQEmbeddedParameterEditor(group_, rel, viewer_);
        }

        if (field)
        {
            auto *l = new QLabel(label);
            l->setToolTip(tip);
            l->setBuddy(field);
            form_->addRow(l, field);
        }
        if (nested)
        {
            form_->addRow(nested);
        }
    }
}




void IQParameterChildrenForm::refresh()
{
    auto sig = currentSignature();
    if (sig!=signature_ || (!sig.empty() && form_->rowCount()==0))
    {
        signature_=sig;
        rebuild();
    }
    if (signature_.empty())
        hide();
    else if (parentWidget())
        show();
}
