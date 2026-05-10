#include "iqchartinteractivelegend.h"
#include "iqgraphprogresschart.h"

#include <QCheckBox>
#include <QStyle>

class FlowLayout
    : public QLayout
{

    QList<QLayoutItem *> itemList;
    int m_hSpace, m_vSpace;

public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout()
    {
        while (auto *item = takeAt(0))
            delete item;
    }

    void addItem(QLayoutItem *item) override
    {
        itemList.append(item);
    }

    bool hasHeightForWidth() const override
    {
        return true;
    }

    int heightForWidth(int width) const override
    {
        return doLayout(QRect(0, 0, width, 0), true);
    }

    int count() const override
    {
        return itemList.size();
    }

    QLayoutItem *itemAt(int index) const override
    {
        return itemList.value(index);
    }

    QLayoutItem *takeAt(int index) override
    {
        return (index >= 0 && index < itemList.size())
                   ? itemList.takeAt(index) : nullptr;
    }

    Qt::Orientations expandingDirections() const override
    {
        return { };
    }

    QSize sizeHint() const override
    {
        return minimumSize();
    }

    QSize minimumSize() const override
    {
        QSize size;
        for (QLayoutItem *item : itemList)
        {
            size = size.expandedTo(item->minimumSize());
        }
        return size +
               QSize(
                   2 * contentsMargins().top(),
                   2 * contentsMargins().top()
                );
    }

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const
    {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0;

        for (QLayoutItem *item : itemList) {
            QWidget *wid = item->widget();
            int spaceX = m_hSpace;
            if (spaceX == -1 && wid)
                spaceX = wid->style()->layoutSpacing(
                    QSizePolicy::CheckBox, QSizePolicy::CheckBox, Qt::Horizontal);
            int spaceY = m_vSpace;
            if (spaceY == -1 && wid)
                spaceY = wid->style()->layoutSpacing(
                    QSizePolicy::CheckBox, QSizePolicy::CheckBox, Qt::Vertical);

            int nextX = x + item->sizeHint().width() + spaceX;
            if ( ((nextX - spaceX) > effectiveRect.right())
                 && lineHeight > 0)
            {
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + item->sizeHint().width() + spaceX;
                lineHeight = 0;
            }

            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

            x = nextX;
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        }
        return y + lineHeight - rect.y() + bottom;
    }

};



IQChartInteractiveLegend::IQChartInteractiveLegend(QWidget *parent)
    : QWidget(parent)
{
    layout_=new FlowLayout(this);
    setLayout(layout_);
}




void IQChartInteractiveLegend::addEntry(QString name, IQLineSeriesData *ld)
{
    auto *check = new QCheckBox(name);
    check->setChecked(true);

    // Farbe der Checkbox an Kurve anpassen (optional)
    QPalette p = check->palette();
    p.setColor(QPalette::WindowText, ld->color());
    check->setPalette(p);

    connect( check, &QCheckBox::toggled,
             ld, &IQLineSeriesData::setVisibility );

    connect( ld, &QObject::destroyed,
            check, &QObject::deleteLater );

    layout_->addWidget(check);
}


