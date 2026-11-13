#include "iqiscadchatformat.h"

#include <algorithm>

#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextCharFormat>


namespace IQISCADChatFormat
{


QTextDocumentFragment subduedMarkdown(
    const QString& markdown,
    const QFont& baseFont,
    const QColor& color,
    double scale )
{
    QTextDocument d;
    d.setDefaultFont(baseFont);
    d.setMarkdown(markdown);

    const double basePt = baseFont.pointSizeF()>0 ? baseFont.pointSizeF() : 9.;

    // collect first, then format (formatting changes the fragments)
    struct Range { int pos, len; QTextCharFormat fmt; };
    QList<Range> ranges;
    for (auto b=d.begin(); b.isValid(); b=b.next())
    {
        for (auto it=b.begin(); !it.atEnd(); ++it)
        {
            auto f=it.fragment();
            if (!f.isValid())
                continue;
            QTextCharFormat cf=f.charFormat();

            // headings are marked by a relative size adjustment (0: normal, 1: large, ...)
            double pt = cf.hasProperty(QTextFormat::FontPointSize) ? cf.fontPointSize() : basePt;
            if (cf.hasProperty(QTextFormat::FontSizeAdjustment))
            {
                int adj=cf.intProperty(QTextFormat::FontSizeAdjustment);
                pt*=std::max(0.6, 1.0+0.2*adj);
                cf.clearProperty(QTextFormat::FontSizeAdjustment);
            }
            cf.setFontPointSize(pt*scale);
            cf.setForeground(color);
            ranges.append({f.position(), f.length(), cf});
        }
    }

    QTextCursor c(&d);
    for (const auto& r: ranges)
    {
        c.setPosition(r.pos);
        c.setPosition(r.pos+r.len, QTextCursor::KeepAnchor);
        c.setCharFormat(r.fmt);
    }

    c.select(QTextCursor::Document);
    return c.selection();
}


bool replaceMarker(QTextDocument* doc, const QString& marker, const QTextDocumentFragment& fragment)
{
    QTextCursor c=doc->find(marker);
    if (c.isNull())
        return false;
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    c.insertFragment(fragment);
    return true;
}


QColor subduedColor(const QColor& text, const QColor& background, double weight)
{
    auto mix=[weight](int a, int b) { return int(a*(1.-weight)+b*weight+0.5); };
    return QColor(
        mix(text.red(), background.red()),
        mix(text.green(), background.green()),
        mix(text.blue(), background.blue()) );
}


}
