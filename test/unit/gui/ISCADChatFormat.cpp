/*
 * formatting of the ISCAD assistant chat transcript (system prompt display)
 */

#include <QtTest/QtTest>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>

#include "iqiscadchatformat.h"


class ISCADChatFormatTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void subduedColor()
    {
        QCOMPARE(IQISCADChatFormat::subduedColor(Qt::black, Qt::white), QColor(140, 140, 140));
        // dark theme: between light text and dark background
        QCOMPARE(IQISCADChatFormat::subduedColor(Qt::white, Qt::black), QColor(115, 115, 115));
    }


    void subduedMarkdown()
    {
        QFont base;
        base.setPointSizeF(10.);
        QColor gray(140, 140, 140);
        auto frag=IQISCADChatFormat::subduedMarkdown(
            "# Title\n\nSome **bold** text\n\n```\ncode\n```\n", base, gray, 0.8);
        QVERIFY(!frag.isEmpty());

        QTextDocument d;
        d.setDefaultFont(base);
        QTextCursor(&d).insertFragment(frag);
        QVERIFY(d.toPlainText().contains("Title"));
        QVERIFY(d.toPlainText().contains("code"));

        int n=0;
        double titleSize=0, textSize=0;
        bool boldSeen=false;
        for (auto b=d.begin(); b.isValid(); b=b.next())
        {
            for (auto it=b.begin(); !it.atEnd(); ++it)
            {
                auto f=it.fragment();
                auto cf=f.charFormat();
                QCOMPARE(cf.foreground().color(), gray);
                QVERIFY(!cf.hasProperty(QTextFormat::FontSizeAdjustment));
                if (f.text()=="Title")
                    titleSize=cf.fontPointSize();
                if (f.text().startsWith("Some"))
                    textSize=cf.fontPointSize();
                if (f.text()=="bold")
                    boldSeen = cf.fontWeight()>=QFont::Bold;
                ++n;
            }
        }
        QVERIFY(n>=4);
        QCOMPARE(textSize, 8.0);
        QVERIFY(titleSize>textSize); // headings stay larger
        QVERIFY(boldSeen); // markdown emphasis is kept
    }


    void replaceMarker()
    {
        QTextDocument d;
        d.setMarkdown("before\n\n@@M0@@\n\nafter\n");
        QFont base;
        auto frag=IQISCADChatFormat::subduedMarkdown("prompt *text*", base, Qt::gray);

        QVERIFY(IQISCADChatFormat::replaceMarker(&d, "@@M0@@", frag));
        QString t=d.toPlainText();
        QVERIFY(!t.contains("@@M0@@"));
        int b=t.indexOf("before"), p=t.indexOf("prompt text"), a=t.indexOf("after");
        QVERIFY(b>=0 && p>b && a>p);

        // the remaining transcript keeps its format
        auto ab=d.findBlock(a);
        QVERIFY(ab.begin().fragment().charFormat().foreground().color()!=QColor(Qt::gray));

        QVERIFY(!IQISCADChatFormat::replaceMarker(&d, "@@M1@@", frag));
    }
};


QTEST_MAIN(ISCADChatFormatTest)
#include "ISCADChatFormat.moc"
