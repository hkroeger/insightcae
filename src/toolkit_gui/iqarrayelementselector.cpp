#include "iqarrayelementselector.h"

#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QSignalBlocker>

#include "base/parameters/arrayparameter.h"
#include "base/parameters/labeledarrayparameter.h"




IQArrayElementSelector::IQArrayElementSelector(
    IQParameterBindingGroup *group,
    const std::string &relativeArrayPath,
    Mode mode,
    int buttons,
    QWidget *parent )
    : QWidget(parent),
      group_(group),
      relativeArrayPath_(relativeArrayPath),
      mode_(mode)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0,0,0,0);

    auto *selRow = new QHBoxLayout;
    l->addLayout(selRow);

    if (mode_==ComboBox)
    {
        combo_=new QComboBox;
        combo_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        selRow->addWidget(combo_, 1);
        connect(combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &IQArrayElementSelector::emitIfChanged);
    }
    else
    {
        list_=new QListWidget;
        selRow->addWidget(list_, 1);
        connect(list_, &QListWidget::currentRowChanged,
                this, &IQArrayElementSelector::emitIfChanged);
    }

    QBoxLayout* btnLayout;
    if (mode_==ComboBox)
        btnLayout=selRow;
    else
    {
        btnLayout=new QHBoxLayout;
        l->addLayout(btnLayout);
    }

    auto addButton = [&](int flag, QPushButton*& btn, const QString& label, const QString& tip, void (IQArrayElementSelector::*slot)())
    {
        if (buttons & flag)
        {
            btn=new QPushButton(label);
            btn->setToolTip(tip);
            btnLayout->addWidget(btn);
            connect(btn, &QPushButton::clicked, this, slot);
        }
    };
    addButton(Add, addBtn_, tr("Add"), tr("Add a new element"), &IQArrayElementSelector::addElement);
    addButton(Duplicate, duplicateBtn_, tr("Duplicate"), tr("Append a copy of the selected element"), &IQArrayElementSelector::duplicateElement);
    addButton(Rename, renameBtn_, tr("Rename"), tr("Change the label of the selected element"), &IQArrayElementSelector::renameElement);
    addButton(Remove, removeBtn_, tr("Remove"), tr("Remove the selected element"), &IQArrayElementSelector::removeElement);
    if (mode_==List) btnLayout->addStretch();

    connect(group_, &IQParameterBindingGroup::refreshed,
            this, &IQArrayElementSelector::refresh);

    group_->observer()->scheduleNotification();
}




void IQArrayElementSelector::setLabelFunction(LabelFunction lf)
{
    labelFunction_=lf;
    group_->observer()->scheduleNotification();
}




const insight::Parameter *IQArrayElementSelector::arrayParameter() const
{
    auto *p = group_->parameter(relativeArrayPath_);
    if ( dynamic_cast<const insight::ArrayParameter*>(p)
        || dynamic_cast<const insight::LabeledArrayParameter*>(p) )
        return p;
    return nullptr;
}




bool IQArrayElementSelector::isLabeled() const
{
    return dynamic_cast<const insight::LabeledArrayParameter*>(arrayParameter());
}




bool IQArrayElementSelector::keysLocked() const
{
    if (auto *lap = dynamic_cast<const insight::LabeledArrayParameter*>(arrayParameter()))
        return lap->keysAreLocked();
    return false;
}




int IQArrayElementSelector::viewRow() const
{
    return combo_ ? combo_->currentIndex() : list_->currentRow();
}




void IQArrayElementSelector::setViewRow(int r)
{
    if (combo_)
        combo_->setCurrentIndex(r);
    else
        list_->setCurrentRow(r);
}




void IQArrayElementSelector::setItems(const QStringList &labels)
{
    if (combo_)
    {
        bool same = (combo_->count()==labels.size());
        for (int i=0; same && i<labels.size(); ++i)
            same = (combo_->itemText(i)==labels[i]);
        if (!same)
        {
            combo_->clear();
            combo_->addItems(labels);
        }
    }
    else
    {
        bool same = (list_->count()==labels.size());
        for (int i=0; same && i<labels.size(); ++i)
            same = (list_->item(i)->text()==labels[i]);
        if (!same)
        {
            list_->clear();
            list_->addItems(labels);
        }
    }
}




int IQArrayElementSelector::currentRow() const
{
    int r=viewRow();
    if (r>=0 && r<int(keys_.size()))
        return r;
    return -1;
}




std::string IQArrayElementSelector::currentKey() const
{
    int r=currentRow();
    if (r>=0)
        return keys_[r];
    return std::string();
}




std::string IQArrayElementSelector::currentElementPath() const
{
    if (!arrayParameter() || currentRow()<0)
        return std::string();
    return joinParameterPath(
        group_->fullPath(relativeArrayPath_),
        currentKey() );
}




void IQArrayElementSelector::setCurrentRow(int row)
{
    setViewRow(row);
    emitIfChanged();
}




void IQArrayElementSelector::setCurrentElementPath(const std::string &path)
{
    if (path.empty() || path==currentElementPath())
        return;

    auto arrayPath = group_->fullPath(relativeArrayPath_)+"/";
    if (path.compare(0, arrayPath.size(), arrayPath)==0)
    {
        auto key = path.substr(arrayPath.size());
        if (key.find('/')==std::string::npos)
            selectKey(key);
    }
}




void IQArrayElementSelector::selectKey(const std::string &key)
{
    auto i = std::find(keys_.begin(), keys_.end(), key);
    if (i!=keys_.end())
        setCurrentRow(int(i-keys_.begin()));
}




void IQArrayElementSelector::emitIfChanged()
{
    auto cp = currentElementPath();
    bool valid = !cp.empty();

    if (removeBtn_) removeBtn_->setEnabled(valid && !keysLocked());
    if (duplicateBtn_) duplicateBtn_->setEnabled(valid && !keysLocked());
    if (renameBtn_) renameBtn_->setEnabled(valid && isLabeled() && !keysLocked());

    if (cp!=emittedPath_ || valid!=emittedValid_)
    {
        emittedPath_=cp;
        emittedValid_=valid;
        Q_EMIT currentElementChanged(cp);
    }
}




void IQArrayElementSelector::refresh()
{
    auto prevKey = currentKey();
    int prevRow = currentRow();

    keys_.clear();
    QStringList labels;

    auto *ap = arrayParameter();
    if (ap)
    {
        bool labeled = isLabeled();
        for (int i=0; i<ap->nChildren(); ++i)
        {
            auto key = ap->childElementName(i);
            keys_.push_back(key);

            auto& e = dynamic_cast<const insight::Parameter&>(ap->childElement(i));
            if (labelFunction_)
                labels.append(labelFunction_(e, i, key));
            else if (labeled)
                labels.append(QString::fromStdString(key));
            else
                labels.append(QString("#%1").arg(i+1));
        }
    }

    {
        QSignalBlocker sb1(combo_), sb2(list_);
        setItems(labels);

        // restore selection
        int row=-1;
        if (isLabeled())
        {
            auto i=std::find(keys_.begin(), keys_.end(), prevKey);
            if (i!=keys_.end()) row=int(i-keys_.begin());
        }
        else
        {
            row=prevRow;
        }
        if (row<0 && !keys_.empty())
            row=std::max(0, std::min(prevRow, int(keys_.size())-1));
        if (row>=int(keys_.size()))
            row=int(keys_.size())-1;
        setViewRow(row);
    }

    setEnabled(ap!=nullptr);
    if (addBtn_) addBtn_->setEnabled(ap && !keysLocked());

    emitIfChanged();
}




void IQArrayElementSelector::addElement()
{
    auto path = group_->fullPath(relativeArrayPath_);

    if (auto *lap = dynamic_cast<const insight::LabeledArrayParameter*>(arrayParameter()))
    {
        bool ok=false;
        auto label = QInputDialog::getText(
            this, tr("New element"), tr("Label of the new element:"),
            QLineEdit::Normal,
            QString::fromStdString(lap->findUniqueNewKey()), &ok );
        if (!ok || label.isEmpty())
            return;

        auto key = label.toStdString();
        if (modifyParameterAs<insight::LabeledArrayParameter>(
                group_->model(), path,
                [&key](insight::LabeledArrayParameter& p) { p.insertWithDefaults(key); },
                this ))
        {
            refresh();
            selectKey(key);
        }
    }
    else if (arrayParameter())
    {
        if (modifyParameterAs<insight::ArrayParameter>(
                group_->model(), path,
                [](insight::ArrayParameter& p) { p.appendEmpty(); },
                this ))
        {
            refresh();
            setCurrentRow(int(keys_.size())-1);
        }
    }
}




void IQArrayElementSelector::removeElement()
{
    auto path = group_->fullPath(relativeArrayPath_);
    int row = currentRow();
    if (row<0) return;
    auto key = currentKey();

    bool ok;
    if (isLabeled())
    {
        ok=modifyParameterAs<insight::LabeledArrayParameter>(
            group_->model(), path,
            [&key](insight::LabeledArrayParameter& p) { p.eraseValue(key); },
            this );
    }
    else
    {
        ok=modifyParameterAs<insight::ArrayParameter>(
            group_->model(), path,
            [row](insight::ArrayParameter& p) { p.eraseValue(row); },
            this );
    }

    if (ok)
    {
        refresh();
    }
}




void IQArrayElementSelector::duplicateElement()
{
    auto path = group_->fullPath(relativeArrayPath_);
    int row = currentRow();
    if (row<0) return;

    if (auto *lap = dynamic_cast<const insight::LabeledArrayParameter*>(arrayParameter()))
    {
        auto key = lap->findUniqueNewKey();
        auto srcKey = currentKey();
        if (modifyParameterAs<insight::LabeledArrayParameter>(
                group_->model(), path,
                [&key,&srcKey](insight::LabeledArrayParameter& p)
                {
                    p.insertValue(key, p[srcKey].cloneAs<insight::Parameter>());
                },
                this ))
        {
            refresh();
            selectKey(key);
        }
    }
    else if (arrayParameter())
    {
        if (modifyParameterAs<insight::ArrayParameter>(
                group_->model(), path,
                [row](insight::ArrayParameter& p)
                {
                    p.appendValue(p.element(row).cloneAs<insight::Parameter>());
                },
                this ))
        {
            refresh();
            setCurrentRow(int(keys_.size())-1);
        }
    }
}




void IQArrayElementSelector::renameElement()
{
    if (!isLabeled() || currentRow()<0)
        return;

    auto oldKey = currentKey();
    bool ok=false;
    auto label = QInputDialog::getText(
        this, tr("Rename element"), tr("New label:"),
        QLineEdit::Normal,
        QString::fromStdString(oldKey), &ok );
    if (!ok || label.isEmpty() || label.toStdString()==oldKey)
        return;

    auto newKey = label.toStdString();
    if (modifyParameterAs<insight::LabeledArrayParameter>(
            group_->model(), group_->fullPath(relativeArrayPath_),
            [&oldKey,&newKey](insight::LabeledArrayParameter& p)
            {
                p.changeLabel(oldKey, newKey);
            },
            this ))
    {
        refresh();
        selectKey(newKey);
    }
}
