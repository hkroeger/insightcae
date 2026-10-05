#include "iqparameterbinding.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QSignalBlocker>

#include <algorithm>

#include "base/parameters/simpleparameter.h"
#include "base/parameters/selectionparameter.h"
#include "base/linearalgebra.h"




namespace {

QDoubleValidator* newDoubleValidator(QObject* parent)
{
    auto *v = new QDoubleValidator(parent);
    v->setLocale(QLocale::c()); // values are converted using C locale
    return v;
}

QString formatDouble(double v)
{
    return QString::number(v, 'g', 12);
}

// never show widgets, which are not yet placed in some parent:
// they would appear as top level windows
void setWidgetVisible(QWidget* w, bool visible)
{
    if (!visible || w->parentWidget())
        w->setVisible(visible);
}

}




IQParameterBinding::IQParameterBinding(
    IQParameterBindingGroup *group,
    const std::string &relativePath,
    QWidget *widget )
    : QObject(group),
      group_(group),
      relativePath_(relativePath),
      widget_(widget)
{}




IQParameterBindingGroup *IQParameterBinding::group() const
{
    return group_;
}




const std::string &IQParameterBinding::relativePath() const
{
    return relativePath_;
}




std::string IQParameterBinding::fullPath() const
{
    return group_->fullPath(relativePath_);
}




QWidget *IQParameterBinding::widget() const
{
    return widget_;
}




IQParameterBinding *IQParameterBinding::alsoAffect(QWidget *w)
{
    companions_.push_back(w);
    return this;
}




IQParameterBinding *IQParameterBinding::setHideWhenUnresolved(bool hide)
{
    hideWhenUnresolved_=hide;
    return this;
}




void IQParameterBinding::write(std::function<void (insight::Parameter &)> f)
{
    if (!group_->isActive())
        return;

    group_->modify(relativePath_, f, widget_);
}




bool IQParameterBinding::userIsEditing() const
{
    return false;
}




void IQParameterBinding::refresh()
{
    if (!widget_)
        return;

    auto *p = group_->isActive() ?
                  group_->parameter(relativePath_) : nullptr;

    std::vector<QWidget*> affected{widget_};
    for (auto& c: companions_)
        if (c) affected.push_back(c);

    for (auto *w: affected)
    {
        if (hideWhenUnresolved_)
        {
            setWidgetVisible(w, p!=nullptr);
            w->setEnabled(true);
        }
        else
        {
            w->setEnabled(p!=nullptr);
        }
    }

    if (p && !userIsEditing())
    {
        QSignalBlocker sb(widget_);
        widget_->setToolTip(
            QString::fromStdString(p->description().toPlainText()) );
        try
        {
            readFromParameter(*p);
        }
        catch (const std::bad_cast&)
        {
            widget_->setEnabled(false);
        }
    }
}




IQDoubleBinding::IQDoubleBinding(
    IQParameterBindingGroup *group,
    const std::string &path,
    QLineEdit *le )
    : IQParameterBinding(group, path, le)
{
    le->setValidator(newDoubleValidator(le));
    connect(le, &QLineEdit::editingFinished, this,
            [this,le]()
            {
                if (!le->isModified()) return;
                le->setModified(false);
                double v = le->text().toDouble();
                writeAs<insight::DoubleParameter>(
                    [v](insight::DoubleParameter& p) { p.set(v); } );
            });
}

void IQDoubleBinding::readFromParameter(const insight::Parameter &p)
{
    auto *le=static_cast<QLineEdit*>(widget());
    le->setText(formatDouble(dynamic_cast<const insight::DoubleParameter&>(p)()));
    le->setModified(false);
}

bool IQDoubleBinding::userIsEditing() const
{
    auto *le=static_cast<QLineEdit*>(widget());
    return le->hasFocus() && le->isModified();
}




IQIntBinding::IQIntBinding(
    IQParameterBindingGroup *group,
    const std::string &path,
    QLineEdit *le )
    : IQParameterBinding(group, path, le)
{
    le->setValidator(new QIntValidator(le));
    connect(le, &QLineEdit::editingFinished, this,
            [this,le]()
            {
                if (!le->isModified()) return;
                le->setModified(false);
                int v = le->text().toInt();
                writeAs<insight::IntParameter>(
                    [v](insight::IntParameter& p) { p.set(v); } );
            });
}

void IQIntBinding::readFromParameter(const insight::Parameter &p)
{
    auto *le=static_cast<QLineEdit*>(widget());
    le->setText(QString::number(dynamic_cast<const insight::IntParameter&>(p)()));
    le->setModified(false);
}

bool IQIntBinding::userIsEditing() const
{
    auto *le=static_cast<QLineEdit*>(widget());
    return le->hasFocus() && le->isModified();
}




IQStringBinding::IQStringBinding(
    IQParameterBindingGroup *group,
    const std::string &path,
    QLineEdit *le )
    : IQParameterBinding(group, path, le)
{
    connect(le, &QLineEdit::editingFinished, this,
            [this,le]()
            {
                if (!le->isModified()) return;
                le->setModified(false);
                auto v = le->text().toStdString();
                writeAs<insight::StringParameter>(
                    [&v](insight::StringParameter& p) { p.set(v); } );
            });
}

void IQStringBinding::readFromParameter(const insight::Parameter &p)
{
    auto *le=static_cast<QLineEdit*>(widget());
    le->setText(QString::fromStdString(
        dynamic_cast<const insight::StringParameter&>(p)() ));
    le->setModified(false);
}

bool IQStringBinding::userIsEditing() const
{
    auto *le=static_cast<QLineEdit*>(widget());
    return le->hasFocus() && le->isModified();
}




IQBoolBinding::IQBoolBinding(
    IQParameterBindingGroup *group,
    const std::string &path,
    QCheckBox *cb )
    : IQParameterBinding(group, path, cb)
{
    connect(cb, &QCheckBox::clicked, this,
            [this](bool checked)
            {
                writeAs<insight::BoolParameter>(
                    [checked](insight::BoolParameter& p) { p.set(checked); } );
            });
}

void IQBoolBinding::readFromParameter(const insight::Parameter &p)
{
    static_cast<QCheckBox*>(widget())->setChecked(
        dynamic_cast<const insight::BoolParameter&>(p)() );
}




IQVectorBinding::IQVectorBinding(
    IQParameterBindingGroup *group,
    const std::string &path,
    QWidget *parent )
    : IQParameterBinding(group, path, new QWidget(parent))
{
    auto *l = new QHBoxLayout(widget());
    l->setContentsMargins(0,0,0,0);
    for (const char* c: {"x", "y", "z"})
    {
        auto *le = new QLineEdit(widget());
        le->setPlaceholderText(c);
        le->setValidator(newDoubleValidator(le));
        l->addWidget(le);
        components_.push_back(le);

        connect(le, &QLineEdit::editingFinished, this,
                [this,le]()
                {
                    if (!le->isModified()) return;

                    arma::mat v = insight::vec3Zero();
                    for (int i=0; i<3; ++i)
                    {
                        components_[i]->setModified(false);
                        v(i) = components_[i]->text().toDouble();
                    }
                    writeAs<insight::VectorParameter>(
                        [&v](insight::VectorParameter& p) { p.set(v); } );
                });
    }
}

void IQVectorBinding::readFromParameter(const insight::Parameter &p)
{
    const arma::mat& v = dynamic_cast<const insight::VectorParameter&>(p)();
    for (int i=0; i<3; ++i)
    {
        QSignalBlocker sb(components_[i]);
        components_[i]->setText(
            i<int(v.n_elem) ? formatDouble(v(i)) : QString() );
        components_[i]->setModified(false);
    }
}

bool IQVectorBinding::userIsEditing() const
{
    for (auto *c: components_)
        if (c->hasFocus() && c->isModified())
            return true;
    return false;
}




IQSelectionBinding::IQSelectionBinding(
    IQParameterBindingGroup *group,
    const std::string &path,
    QComboBox *cb,
    std::function<QString(const std::string&)> keyLabel )
    : IQParameterBinding(group, path, cb),
      keyLabel_(keyLabel)
{
    connect(cb, QOverload<int>::of(&QComboBox::activated), this,
            [this,cb](int i)
            {
                auto key = cb->itemData(i).toString().toStdString();
                write(
                    [&key](insight::Parameter& p)
                    {
                        auto& sp = dynamic_cast<insight::SelectionParameterInterface&>(p);
                        if (sp.selection()!=key)
                            sp.setSelection(key);
                    });
            });
}

void IQSelectionBinding::readFromParameter(const insight::Parameter &p)
{
    auto *cb=static_cast<QComboBox*>(widget());
    auto& sp = dynamic_cast<const insight::SelectionParameterInterface&>(p);

    auto keys = sp.selectionKeys();

    bool same = (cb->count()==int(keys.size()));
    for (int i=0; same && i<cb->count(); ++i)
        same = (cb->itemData(i).toString().toStdString()==keys[i]);

    if (!same)
    {
        cb->clear();
        for (const auto& k: keys)
        {
            cb->addItem(
                keyLabel_ ? keyLabel_(k) : QString::fromStdString(k),
                QString::fromStdString(k) );
        }
    }

    cb->setCurrentIndex(
        cb->findData(QString::fromStdString(sp.selection())) );
}




IQParameterBindingGroup::IQParameterBindingGroup(
    IQParameterSetModel *model,
    QObject *parent,
    const std::string &basePath )
    : QObject(parent),
      model_(model),
      observer_(new IQParameterSetModelObserver(model, this)),
      basePath_(basePath),
      active_(true)
{
    connect(observer_, &IQParameterSetModelObserver::parameterSetChanged,
            this, &IQParameterBindingGroup::refresh);
}




IQParameterSetModel *IQParameterBindingGroup::model() const
{
    return model_;
}




IQParameterSetModelObserver *IQParameterBindingGroup::observer() const
{
    return observer_;
}




const std::string &IQParameterBindingGroup::basePath() const
{
    return basePath_;
}




std::string IQParameterBindingGroup::fullPath(const std::string &relativePath) const
{
    return joinParameterPath(basePath_, relativePath);
}




bool IQParameterBindingGroup::isActive() const
{
    return active_;
}




void IQParameterBindingGroup::setBasePath(const std::string &basePath)
{
    bool changed = (!active_) || (basePath!=basePath_);
    basePath_=basePath;
    active_=true;
    if (changed)
    {
        refresh();
        Q_EMIT basePathChanged(basePath_);
    }
}




void IQParameterBindingGroup::clearBasePath()
{
    bool changed = active_;
    basePath_.clear();
    active_=false;
    if (changed)
    {
        refresh();
        Q_EMIT basePathChanged(basePath_);
    }
}




bool IQParameterBindingGroup::hasParameter(const std::string &relativePath) const
{
    return parameter(relativePath)!=nullptr;
}




const insight::Parameter *IQParameterBindingGroup::parameter(
    const std::string &relativePath ) const
{
    if (!active_)
        return nullptr;
    return findParameter(model_, fullPath(relativePath));
}




bool IQParameterBindingGroup::modify(
    const std::string &relativePath,
    std::function<void (insight::Parameter &)> modification,
    QWidget *messageParent )
{
    if (!active_)
        return false;
    return modifyParameter(
        model_, fullPath(relativePath), modification, messageParent );
}




std::string IQParameterBindingGroup::selectionKey(
    const std::string &relativePath ) const
{
    if (auto *sp = dynamic_cast<const insight::SelectionParameterInterface*>(
            parameter(relativePath)))
    {
        return sp->selection();
    }
    return std::string();
}




void IQParameterBindingGroup::addBinding(IQParameterBinding *b)
{
    bindings_.push_back(b);
    // initial update deferred until the widget has been placed
    observer_->scheduleNotification();
}




void IQParameterBindingGroup::addVisibilityRule(
    QWidget *w, std::function<bool ()> isVisible )
{
    visibilityRules_.push_back({w, isVisible});
    observer_->scheduleNotification();
}




void IQParameterBindingGroup::showIfPathExists(
    QWidget *w, const std::string &relativePath )
{
    addVisibilityRule(
        w, [this,relativePath]() { return hasParameter(relativePath); } );
}




void IQParameterBindingGroup::showIfSelected(
    QWidget *w, const std::string &relativePath, const std::string &key )
{
    addVisibilityRule(
        w, [this,relativePath,key]() { return selectionKey(relativePath)==key; } );
}




QLineEdit *IQParameterBindingGroup::bindDouble(
    const std::string &relativePath, QLineEdit *le )
{
    if (!le) le=new QLineEdit;
    addBinding(new IQDoubleBinding(this, relativePath, le));
    return le;
}




QLineEdit *IQParameterBindingGroup::bindInt(
    const std::string &relativePath, QLineEdit *le )
{
    if (!le) le=new QLineEdit;
    addBinding(new IQIntBinding(this, relativePath, le));
    return le;
}




QLineEdit *IQParameterBindingGroup::bindString(
    const std::string &relativePath, QLineEdit *le )
{
    if (!le) le=new QLineEdit;
    addBinding(new IQStringBinding(this, relativePath, le));
    return le;
}




QCheckBox *IQParameterBindingGroup::bindBool(
    const std::string &relativePath, QCheckBox *cb )
{
    if (!cb) cb=new QCheckBox;
    addBinding(new IQBoolBinding(this, relativePath, cb));
    return cb;
}




QComboBox *IQParameterBindingGroup::bindSelection(
    const std::string &relativePath, QComboBox *cb,
    std::function<QString (const std::string &)> keyLabel )
{
    if (!cb) cb=new QComboBox;
    addBinding(new IQSelectionBinding(this, relativePath, cb, keyLabel));
    return cb;
}




QWidget *IQParameterBindingGroup::bindVector(const std::string &relativePath)
{
    auto *b = new IQVectorBinding(this, relativePath);
    addBinding(b);
    return b->widget();
}




void IQParameterBindingGroup::refresh()
{
    // remove deleted bindings
    bindings_.erase(
        std::remove_if(bindings_.begin(), bindings_.end(),
                       [](const QPointer<IQParameterBinding>& b) { return b.isNull(); }),
        bindings_.end() );
    visibilityRules_.erase(
        std::remove_if(visibilityRules_.begin(), visibilityRules_.end(),
                       [](const VisibilityRule& r) { return r.widget.isNull(); }),
        visibilityRules_.end() );

    // copy: bindings might be added during refresh
    auto bindings = bindings_;
    for (auto& b: bindings)
    {
        if (b) b->refresh();
    }

    for (auto& vr: visibilityRules_)
    {
        if (vr.widget)
            setWidgetVisible(vr.widget, active_ && vr.isVisible());
    }

    Q_EMIT refreshed();
}
