#include "iqembeddedparametereditor.h"

#include <QVBoxLayout>

#include "iqparameter.h"




IQEmbeddedParameterEditor::IQEmbeddedParameterEditor(
    IQParameterBindingGroup *group,
    const std::string &relativePath,
    IQCADModel3DViewer *viewer,
    QWidget *parent )
    : QWidget(parent),
      group_(group),
      relativePath_(relativePath),
      viewer_(viewer),
      contents_(nullptr)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0,0,0,0);

    connect(group_, &IQParameterBindingGroup::refreshed,
            this, &IQEmbeddedParameterEditor::refresh);

    // initial update, once placed into its parent
    group_->observer()->scheduleNotification();
}




void IQEmbeddedParameterEditor::rebuild(IQParameter *p)
{
    if (contents_)
    {
        contents_->deleteLater();
        contents_=nullptr;
    }

    currentParameter_=p;

    if (p)
    {
        contents_ = new QWidget(this);
        layout()->addWidget(contents_);
        p->populateEditControls(contents_, viewer_);
        p->checkEnabledOrDisabled();
    }
}




void IQEmbeddedParameterEditor::refresh()
{
    IQParameter* p = nullptr;
    std::string path;

    if (group_->hasParameter(relativePath_))
    {
        path = group_->fullPath(relativePath_);
        p = ensureParameterWrapper(group_->model(), path);
    }

    // wrappers are deleted along with their parameter
    // but the address of a new wrapper might be identical:
    // compare path as well
    if ( (p != currentParameter_.data())
        || (path != currentPath_)
        || (p && !contents_) )
    {
        currentPath_=path;
        rebuild(p);
    }

    if (!p)
        hide();
    else if (parentWidget())
        show();
}
