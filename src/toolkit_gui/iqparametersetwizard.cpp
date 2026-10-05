#include "iqparametersetwizard.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>




IQParameterSetWizardPage::IQParameterSetWizardPage(IQParameterSetWizard *wizard)
    : QWidget(),
      wizard_(wizard),
      bindings_(new IQParameterBindingGroup(wizard->model(), this))
{}




IQParameterSetWizard *IQParameterSetWizardPage::wizard() const
{
    return wizard_;
}




IQParameterSetModel *IQParameterSetWizardPage::model() const
{
    return wizard_->model();
}




IQCADModel3DViewer *IQParameterSetWizardPage::viewer() const
{
    return wizard_->viewer();
}




IQParameterBindingGroup *IQParameterSetWizardPage::bindings() const
{
    return bindings_;
}




IQParameterBindingGroup *IQParameterSetWizardPage::createBindingGroup()
{
    auto *g = new IQParameterBindingGroup(model(), this);
    g->clearBasePath();
    return g;
}




IQEmbeddedParameterEditor *IQParameterSetWizardPage::createEmbeddedEditor(
    IQParameterBindingGroup *group,
    const std::string &relativePath )
{
    return new IQEmbeddedParameterEditor(group, relativePath, viewer());
}




IQArrayElementSelector *IQParameterSetWizardPage::createContextSelector(
    const std::string &contextName,
    IQParameterBindingGroup *group,
    const std::string &relativeArrayPath,
    IQArrayElementSelector::Mode mode,
    int buttons )
{
    auto *sel = new IQArrayElementSelector(
        group, relativeArrayPath, mode, buttons );

    connect(sel, &IQArrayElementSelector::currentElementChanged, wizard_,
            [this,contextName](const std::string& path)
            {
                if (!path.empty())
                    wizard_->setContextValue(contextName, path);
            });

    connect(wizard_, &IQParameterSetWizard::contextChanged, sel,
            [sel,contextName](const std::string& name, const std::string& value)
            {
                if (name==contextName)
                    sel->setCurrentElementPath(value);
            });

    // adopt the current context, once the element list is available
    connect(group, &IQParameterBindingGroup::refreshed, sel,
            [this,sel,contextName]()
            {
                sel->setCurrentElementPath(wizard_->contextValue(contextName));
            });

    return sel;
}




IQParameterBindingGroup *IQParameterSetWizardPage::createDetailGroup(
    IQArrayElementSelector *selector )
{
    auto *g = createBindingGroup();
    auto follow = [g](const std::string& path)
    {
        if (path.empty())
            g->clearBasePath();
        else
            g->setBasePath(path);
    };
    connect(selector, &IQArrayElementSelector::currentElementChanged, g, follow);
    follow(selector->currentElementPath()); // selection might already exist
    return g;
}




QString IQParameterSetWizardPage::humanize(const std::string &key)
{
    QString r;
    for (auto c: QString::fromStdString(key))
    {
        if (c.isUpper() && !r.isEmpty())
        {
            r+=' ';
            r+=c.toLower();
        }
        else if (c=='_')
            r+=' ';
        else
            r+=c;
    }
    if (!r.isEmpty())
        r[0]=r[0].toUpper();
    return r;
}




void IQParameterSetWizardPage::addBindingRow(
    QFormLayout *form, const QString &label, IQParameterBinding *binding )
{
    auto *l = new QLabel(label);
    l->setBuddy(binding->widget());
    binding->setHideWhenUnresolved(true)->alsoAffect(l);
    form->addRow(l, binding->widget());
    binding->group()->addBinding(binding);
}




QLineEdit *IQParameterSetWizardPage::addDoubleRow(
    QFormLayout *form, IQParameterBindingGroup *group,
    const QString &label, const std::string &relativePath )
{
    auto *le = new QLineEdit;
    addBindingRow(form, label, new IQDoubleBinding(group, relativePath, le));
    return le;
}




QLineEdit *IQParameterSetWizardPage::addIntRow(
    QFormLayout *form, IQParameterBindingGroup *group,
    const QString &label, const std::string &relativePath )
{
    auto *le = new QLineEdit;
    addBindingRow(form, label, new IQIntBinding(group, relativePath, le));
    return le;
}




QLineEdit *IQParameterSetWizardPage::addStringRow(
    QFormLayout *form, IQParameterBindingGroup *group,
    const QString &label, const std::string &relativePath )
{
    auto *le = new QLineEdit;
    addBindingRow(form, label, new IQStringBinding(group, relativePath, le));
    return le;
}




QCheckBox *IQParameterSetWizardPage::addBoolRow(
    QFormLayout *form, IQParameterBindingGroup *group,
    const QString &label, const std::string &relativePath )
{
    auto *cb = new QCheckBox;
    addBindingRow(form, label, new IQBoolBinding(group, relativePath, cb));
    return cb;
}




QWidget *IQParameterSetWizardPage::addVectorRow(
    QFormLayout *form, IQParameterBindingGroup *group,
    const QString &label, const std::string &relativePath )
{
    auto *b = new IQVectorBinding(group, relativePath);
    addBindingRow(form, label, b);
    return b->widget();
}




QComboBox *IQParameterSetWizardPage::addSelectionRow(
    QFormLayout *form, IQParameterBindingGroup *group,
    const QString &label, const std::string &relativePath,
    std::function<QString (const std::string &)> keyLabel )
{
    auto *cb = new QComboBox;
    addBindingRow(form, label,
                  new IQSelectionBinding(
                      group, relativePath, cb,
                      keyLabel ? keyLabel : &IQParameterSetWizardPage::humanize ) );
    return cb;
}




IQParameterSetWizard::IQParameterSetWizard(
    IQParameterSetModel *model,
    IQCADModel3DViewer *viewer,
    const QString& title,
    QWidget *parent )
    : QWidget(parent),
      model_(model),
      viewer_(viewer)
{
    auto *l = new QVBoxLayout(this);

    auto *tl = new QLabel(title);
    auto f=tl->font(); f.setBold(true); tl->setFont(f);
    l->addWidget(tl);

    tabs_ = new QTabWidget;
    l->addWidget(tabs_, 1);

    auto *nav = new QHBoxLayout;
    l->addLayout(nav);
    prevBtn_ = new QPushButton(tr("<< Prev"));
    nav->addWidget(prevBtn_);
    nav->addStretch();
    nextBtn_ = new QPushButton(tr("Next >>"));
    nav->addWidget(nextBtn_);

    connect(prevBtn_, &QPushButton::clicked, tabs_,
            [this]() { tabs_->setCurrentIndex(tabs_->currentIndex()-1); });
    connect(nextBtn_, &QPushButton::clicked, tabs_,
            [this]() { tabs_->setCurrentIndex(tabs_->currentIndex()+1); });
    connect(tabs_, &QTabWidget::currentChanged,
            this, &IQParameterSetWizard::updateNavigationButtons);

    updateNavigationButtons();
}




IQParameterSetModel *IQParameterSetWizard::model() const
{
    return model_;
}




IQCADModel3DViewer *IQParameterSetWizard::viewer() const
{
    return viewer_;
}




void IQParameterSetWizard::updateNavigationButtons()
{
    prevBtn_->setEnabled(tabs_->currentIndex()>0);
    nextBtn_->setEnabled(tabs_->currentIndex()<tabs_->count()-1);
}




void IQParameterSetWizard::addPage(IQParameterSetWizardPage *page, const QString &title)
{
    auto *sa = new QScrollArea;
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setWidget(page);
    tabs_->addTab(sa, title);
    updateNavigationButtons();
}




void IQParameterSetWizard::addTab(QWidget *widget, const QString &title)
{
    tabs_->addTab(widget, title);
    updateNavigationButtons();
}




std::string IQParameterSetWizard::contextValue(const std::string &name) const
{
    auto i=context_.find(name);
    if (i!=context_.end())
        return i->second;
    return std::string();
}




void IQParameterSetWizard::setContextValue(
    const std::string &name, const std::string &value )
{
    auto& v = context_[name];
    if (v!=value)
    {
        v=value;
        Q_EMIT contextChanged(name, value);
    }
}
