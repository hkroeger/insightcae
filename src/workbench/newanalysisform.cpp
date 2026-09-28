#include "newanalysisform.h"
#include "base/analysis.h"
#include "ui_newanalysisform.h"

#include "cadparametersetvisualizer.h"

#include "base/analysis.h"
#include "base/translations.h"

#include <QProxyStyle>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFontMetrics>




// enlargement factor for the tree of available analyses
const double treeScale = 1.5;




// draws large triangular expand/collapse handles
class LargeBranchIndicatorStyle
    : public QProxyStyle
{
public:
    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget = nullptr ) const override
    {
        if ( element==PE_IndicatorBranch && (option->state & State_Children) )
        {
            const QRect& r = option->rect;
            int fh = widget ? QFontMetrics(widget->font()).height() : r.height();
            double s = std::min( 0.5*std::min(r.width(), r.height()), double(fh) );
            QPointF c = QRectF(r).center();

            QPolygonF tri;
            if (option->state & State_Open)
            {
                tri << c+QPointF(-0.5*s, -0.25*s)
                    << c+QPointF( 0.5*s, -0.25*s)
                    << c+QPointF( 0.,     0.25*s);
            }
            else
            {
                tri << c+QPointF(-0.25*s, -0.5*s)
                    << c+QPointF(-0.25*s,  0.5*s)
                    << c+QPointF( 0.25*s,  0.);
            }

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(option->palette.color(QPalette::Text));
            painter->drawPolygon(tri);
            painter->restore();
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};




// adds vertical spacing around each tree item
class PaddedItemDelegate
    : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(
        const QStyleOptionViewItem &option,
        const QModelIndex &index ) const override
    {
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        sz.rheight() += QFontMetrics(option.font).height();
        return sz;
    }
};




NewAnalysisForm::NewAnalysisForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewAnalysisForm)
{
    ui->setupUi(this);

    ui->openBtn->setText(_("&Open..."));
    ui->createBtn->setText(_("&Create selected..."));

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
        { QFont f=parent_->treeWidget()->font(); f.setBold(true); f.setPointSizeF(f.pointSizeF()+5*treeScale); newnode->setFont(0, f); }
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
    { QFont f=treeWidget->font(); f.setPointSizeF(f.pointSizeF()*treeScale); treeWidget->setFont(f); }

    auto *bs = new LargeBranchIndicatorStyle;
    bs->setParent(treeWidget); // setStyle does not take ownership
    treeWidget->setStyle(bs);
    treeWidget->setItemDelegate(new PaddedItemDelegate(treeWidget));

    treeWidget->setColumnCount(2);
    treeWidget->setIconSize(QSize(80,80));
    treeWidget->setWordWrap(true);
    treeWidget->setIndentation(int(1.5*QFontMetrics(treeWidget->font()).height()));
    QStringList hdr;
    hdr << _("Analysis") << _("Description") ;
    treeWidget->setHeaderLabels(hdr);

    QTreeWidgetItem *topitem =
        new QTreeWidgetItem ( treeWidget, QStringList() << _("Available Analyses") );
    { QFont f=treeWidget->font(); f.setBold(true); f.setPointSizeF(f.pointSizeF()+5*treeScale); topitem->setFont(0, f); }
    HierarchyLevel toplevel ( topitem );

    HierarchyLevel::iterator i=toplevel.addHierarchyLevel(_("Uncategorized"));

    auto analyses = insight::Analysis::analyses().ToC();

    for ( const auto& analysisType: analyses )
    {

        QStringList path =
            QString::fromStdString (
                               insight::Analysis::categories()( analysisType ) )
                .split ( "/", Qt::SkipEmptyParts );

        HierarchyLevel* parent = &toplevel;
        for ( QStringList::const_iterator pit = path.constBegin(); pit != path.constEnd(); ++pit )
        {
            parent = & ( parent->sublevel ( pit->toStdString() ) );
        }

        QString desc;
        QIcon icon(":analysis_default_icon.svg");
        if (insight::CADParameterSetModelVisualizer::iconForAnalysis().count(analysisType))
        {
            icon=insight::CADParameterSetModelVisualizer::iconForAnalysis()(analysisType);
        }
        if (insight::Analysis::descriptions().count(analysisType))
        {
            auto d=insight::Analysis::descriptions()(analysisType);
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
        { QFont f=treeWidget->font(); f.setItalic(true); f.setPointSizeF(f.pointSizeF()+1*treeScale); item->setFont(0, f); }
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
