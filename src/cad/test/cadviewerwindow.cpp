#include "cadviewerwindow.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QSplitter>

#include "cadmodel.h"
#include "insightcaeapplication.h"


CADViewerWindow::CADViewerWindow()
    : QMainWindow(),
      itemModel_(std::make_shared<insight::cad::Model>(boost::filesystem::path("testmodel.iscad")))
{

    viewer_=new IQCADModel3DViewer;
    treeView_=new QTreeView;

    auto l = new QSplitter(this);
    setCentralWidget(l);

    l->addWidget(viewer_);
    l->addWidget(treeView_);

    viewer_->setModel(&itemModel_);
    treeView_->setModel(&itemModel_);

    connect(treeView_, &QTreeView::clicked,
            [&](const QModelIndex& idx)
    {
        qDebug()<<idx<<" parent:"<<idx.parent();
    }
    );
}

QSize CADViewerWindow::sizeHint() const
{
    return QSize(1024, 768);
}


int main(int argc, char* argv[])
{
    InsightCAEApplication app ( argc, argv, "CADViewerTest" );

    CADViewerWindow window;

    window.show();

    return app.exec();
}
