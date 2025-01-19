#include "newanalysisform.h"
#include "base/analysis.h"
#include "ui_newanalysisform.h"

#include "cadparametersetvisualizer.h"

#include "base/analysis.h"
#include "base/translations.h"

NewAnalysisForm::NewAnalysisForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewAnalysisForm)
{
    ui->setupUi(this);

    fillAnalysisList(ui->treeWidget);
}

NewAnalysisForm::~NewAnalysisForm()
{
    delete ui;
}

void NewAnalysisForm::replaceLoadButton(QPushButton *b)
{
    delete ui->openBtn;
    ui->horizontalLayout->addWidget(b);
}




class HierarchyLevel
    : public std::map<std::string, HierarchyLevel>
{
public:
    QTreeWidgetItem* parent_;

    HierarchyLevel(QTreeWidgetItem* parent)
        : parent_(parent)
    {}

    iterator addHierarchyLevel(const std::string& entry)
    {
        QTreeWidgetItem* newnode = new QTreeWidgetItem(parent_, QStringList() << entry.c_str());
        { QFont f=newnode->font(0); f.setBold(true); f.setPointSize(f.pointSize()+5); newnode->setFont(0, f); }
        std::pair<iterator,bool> ret = insert(std::make_pair(entry, HierarchyLevel(newnode)));
        return ret.first;
    }

    HierarchyLevel& sublevel(const std::string& entry)
    {
        iterator it = find(entry);
        if (it == end())
        {
            it=addHierarchyLevel(entry);
        }
        return it->second;
    }
};




void NewAnalysisForm::fillAnalysisList(QTreeWidget* treeWidget)
{
    treeWidget->setColumnCount(2);
    treeWidget->setIconSize(QSize(80,80));
    treeWidget->setWordWrap(true);
    treeWidget->setIndentation(5);
    treeWidget->setHeaderLabels(QStringList() << "Analysis" << "Description" );

    QTreeWidgetItem *topitem = new QTreeWidgetItem ( treeWidget, QStringList() << _("Available Analyses") );
    { QFont f=topitem->font(0); f.setBold(true); f.setPointSize(f.pointSize()+5); topitem->setFont(0, f); }
    HierarchyLevel toplevel ( topitem );

    HierarchyLevel::iterator i=toplevel.addHierarchyLevel(_("Uncategorized"));

    auto analyses = insight::Analysis::createAnalysis_table().ToC();

    for ( const auto& analysisType: analyses )
    {

        QStringList path =
            QString::fromStdString (
                               insight::Analysis::categoryFor( analysisType ) )
                .split ( "/", Qt::SkipEmptyParts );

        HierarchyLevel* parent = &toplevel;
        for ( QStringList::const_iterator pit = path.constBegin(); pit != path.constEnd(); ++pit )
        {
            parent = & ( parent->sublevel ( pit->toStdString() ) );
        }

        QString desc;
        QIcon icon(":analysis_default_icon.svg");
        if (insight::CADParameterSetModelVisualizer::iconForAnalysis_table().count(analysisType))
        {
            icon=insight::CADParameterSetModelVisualizer::iconForAnalysis(analysisType);
        }
        if (insight::Analysis::descriptionFor_table().count(analysisType))
        {
            auto d=insight::Analysis::descriptionFor(analysisType);
            try
            {
                desc=QString::fromStdString(
                    insight::SimpleLatex(d.description).toHTML(300));
            }
            catch(...)
            {
                desc=QString::fromStdString(d.description);
            }
        }
        QTreeWidgetItem* item = new QTreeWidgetItem (
            parent->parent_,
            QStringList()
                << analysisType.c_str()
                << desc
            );
        { QFont f=item->font(0); f.setItalic(true); f.setPointSize(f.pointSize()+1); item->setFont(0, f); }
        item->setIcon(0,icon);
        item->setTextAlignment(0, Qt::AlignTop);
    }

    //treeWidget->expandItem(topitem);
    treeWidget->expandAll();
    treeWidget->resizeColumnToContents(0);

    connect(
        treeWidget, &QTreeWidget::itemActivated, treeWidget,
        [this](QTreeWidgetItem *item, int column) {
            if (item->childCount()==0) // don't accept headers
                Q_EMIT createAnalysis(item->text(0).toStdString());
        });

    connect(
        ui->openBtn, &QPushButton::clicked,
        this, &NewAnalysisForm::openAnalysis);

    connect(
        ui->createBtn, &QPushButton::clicked, treeWidget,
        [this,treeWidget]() {
            if (auto si=treeWidget->currentItem())
            {
                if (si->childCount()==0) // don't accept headers
                    Q_EMIT createAnalysis(si->text(0).toStdString());
            }
        });

}
