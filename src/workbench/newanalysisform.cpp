#include "newanalysisform.h"
#include "base/analysis.h"
#include "ui_newanalysisform.h"

#include "cadparametersetvisualizer.h"

#include "base/analysis.h"
#include "base/translations.h"

#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QSignalBlocker>
#include <QResizeEvent>
#include <QTextLayout>

#include <cmath>
#include <map>
#include <algorithm>




// enlargement factor for the gallery of available analyses
const double galleryScale = 1.5;




// breaks a label into at most maxLines lines of given width, elides the last one
static QString wrapLabel(const QString& text, const QFont& font, int width, int maxLines)
{
    QFontMetrics fm(font);
    QTextLayout layout(text, font);
    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(opt);

    QStringList lines;
    layout.beginLayout();
    for (QTextLine line=layout.createLine(); line.isValid(); line=layout.createLine())
    {
        line.setLineWidth(width);
        if (lines.size()==maxLines-1)
        {
            lines << fm.elidedText(text.mid(line.textStart()), Qt::ElideRight, width);
            break;
        }
        lines << text.mid(line.textStart(), line.textLength()).trimmed();
    }
    layout.endLayout();
    return lines.join('\n');
}




// icon gallery which adapts its height to show all items without scrolling
class AnalysisGalleryList
    : public QListWidget
{
    void updateHeight()
    {
        QSize g = gridSize();
        int perRow = std::max( 1, viewport()->width() / std::max(1, g.width()) );
        int rows = std::max( 1, int(std::ceil( double(count()) / double(perRow) )) );
        int h = rows*g.height() + 2*frameWidth() + 4;
        if (h!=height())
            setFixedHeight(h);
    }

public:
    AnalysisGalleryList(QWidget* parent = nullptr)
        : QListWidget(parent)
    {
        setViewMode(QListView::IconMode);
        setFlow(QListView::LeftToRight);
        setWrapping(true);
        setResizeMode(QListView::Adjust);
        setMovement(QListView::Static);
        setDragEnabled(false);
        setWordWrap(true);
        setSpacing(0);
        setFrameShape(QFrame::NoFrame);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    QSize sizeHint() const override
    {
        return QSize( gridSize().width(), height() );
    }

protected:
    void resizeEvent(QResizeEvent* e) override
    {
        QListWidget::resizeEvent(e);
        updateHeight();
    }
};




NewAnalysisForm::NewAnalysisForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewAnalysisForm)
{
    ui->setupUi(this);

    ui->openBtn->setText(_("&Open..."));
    ui->createBtn->setText(_("&Create selected..."));

    fillAnalysisGallery();
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




QListWidgetItem* NewAnalysisForm::selectedItem() const
{
    for (auto* g: galleries_)
    {
        auto si=g->selectedItems();
        if (!si.isEmpty())
            return si.first();
    }
    return nullptr;
}




void NewAnalysisForm::fillAnalysisGallery()
{
    auto *contents = ui->galleryContents;
    { QFont f=contents->font(); f.setPointSizeF(f.pointSizeF()*galleryScale); contents->setFont(f); }

    auto *vl = new QVBoxLayout(contents);

    const QSize iconSize(80,80);
    QFontMetrics fm(contents->font());
    const QSize grid(
        std::max( iconSize.width()+2*fm.height(), 14*fm.averageCharWidth() ),
        iconSize.height() + 3*fm.lineSpacing() + fm.height()/2 );

    // group analyses by category path, uncategorized ones are put last
    typedef std::vector<std::pair<int, QListWidgetItem*> > SectionItems;
    std::map<QString, SectionItems> categorized;
    SectionItems uncategorized;

    auto analyses = insight::Analysis::analyses().ToC();

    for ( const auto& analysisType: analyses )
    {

        QStringList path =
            QString::fromStdString (
                               insight::Analysis::categories()( analysisType ) )
                .split ( "/", Qt::SkipEmptyParts );

        int order = insight::Analysis::defaultGalleryOrder;
        if (insight::Analysis::galleryOrders().count(analysisType))
        {
            order = insight::Analysis::galleryOrders()(analysisType);
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

        // allow line breaks at CamelCase boundaries (zero-width spaces)
        QString label = QString::fromStdString(analysisType);
        for (int k=label.size()-1; k>0; --k)
        {
            if (label[k].isUpper() && label[k-1].isLower())
                label.insert(k, QChar(0x200B));
        }

        auto *item = new QListWidgetItem(
            icon, wrapLabel(label, contents->font(), grid.width()-fm.averageCharWidth(), 3) );
        item->setData(Qt::UserRole, QString::fromStdString(analysisType));
        item->setToolTip(
            QString("<b>%1</b>").arg(QString::fromStdString(analysisType).toHtmlEscaped())
            + (desc.isEmpty() ? QString() : "<br>"+desc) );
        item->setTextAlignment(Qt::AlignHCenter|Qt::AlignTop);

        if (path.isEmpty())
            uncategorized.push_back({order, item});
        else
            categorized[path.join(QString::fromUtf8(" \u203A "))].push_back({order, item});
    }

    // within a section: ascending gallery order, then by name
    auto sortItems = [](SectionItems& items)
    {
        std::stable_sort(
            items.begin(), items.end(),
            [](const SectionItems::value_type& a, const SectionItems::value_type& b)
            { return a.first < b.first; } );
    };

    auto addSection = [&](const QString& title, SectionItems items)
    {
        sortItems(items);

        auto *header = new QToolButton;
        header->setCheckable(true);
        header->setChecked(true);
        header->setAutoRaise(true);
        header->setStyleSheet("QToolButton { border: none; }");
        header->setToolButtonStyle(Qt::ToolButtonTextOnly);
        { QFont f=contents->font(); f.setBold(true); f.setPointSizeF(f.pointSizeF()+2*galleryScale); header->setFont(f); }
        auto setHeaderText = [header,title](bool expanded) {
            header->setText( QString::fromUtf8(expanded ? "\u25BE " : "\u25B8 ") + title );
        };
        setHeaderText(true);
        vl->addWidget(header);

        auto *gallery = new AnalysisGalleryList;
        gallery->setIconSize(iconSize);
        gallery->setGridSize(grid);
        for (auto &item: items)
            gallery->addItem(item.second);
        vl->addWidget(gallery);
        galleries_.push_back(gallery);

        connect(header, &QToolButton::toggled, gallery,
                [setHeaderText,gallery](bool expanded) {
                    setHeaderText(expanded);
                    gallery->setVisible(expanded);
                });

        // only one selected analysis across all groups
        connect(gallery, &QListWidget::itemSelectionChanged, gallery,
                [this,gallery]() {
                    if (gallery->selectedItems().isEmpty()) return;
                    for (auto* g: galleries_)
                    {
                        if (g!=gallery)
                        {
                            QSignalBlocker sb(g);
                            g->clearSelection();
                            g->viewport()->update();
                        }
                    }
                });

        connect(gallery, &QListWidget::itemActivated, gallery,
                [this](QListWidgetItem *item) {
                    Q_EMIT createAnalysis(item->data(Qt::UserRole).toString().toStdString());
                });
    };

    // sections: ascending lowest gallery order of their analyses, then by path
    std::vector<std::pair<int, QString> > sections;
    for (const auto& c: categorized)
    {
        int minOrder = std::min_element(
            c.second.begin(), c.second.end(),
            [](const SectionItems::value_type& a, const SectionItems::value_type& b)
            { return a.first < b.first; } )->first;
        sections.push_back({minOrder, c.first});
    }
    std::sort(sections.begin(), sections.end());

    for (const auto& sec: sections)
        addSection(sec.second, categorized[sec.second]);
    if (!uncategorized.empty())
        addSection(_("Uncategorized"), uncategorized);

    vl->addStretch();

    connect(
        ui->openBtn, &QPushButton::clicked,
        this, &NewAnalysisForm::openAnalysis);

    connect(
        ui->createBtn, &QPushButton::clicked, this,
        [this]() {
            if (auto si=selectedItem())
                Q_EMIT createAnalysis(si->data(Qt::UserRole).toString().toStdString());
        });

}
