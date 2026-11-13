#include "referencedocs.h"

#include <algorithm>

#include <QRegularExpression>

namespace insight {
namespace assistant {


QList<ReferenceSection> splitMarkdownSections(const QString& markdown, int minLevel, int maxLevel)
{
    static const QRegularExpression
        reHeading("^(#{1,6})\\s+(.*?)\\s*#*\\s*$"),
        reFence("^\\s*(```|~~~)");

    const QStringList lines=markdown.split('\n');

    struct Heading { int line, level; QString title; };
    QList<Heading> headings;
    bool inFence=false;
    for (int i=0; i<lines.size(); ++i)
    {
        if (reFence.match(lines[i]).hasMatch())
        {
            inFence=!inFence;
            continue;
        }
        if (inFence)
            continue;
        auto m=reHeading.match(lines[i]);
        if (m.hasMatch())
            headings.append({i, int(m.captured(1).size()), m.captured(2)});
    }

    QList<ReferenceSection> sections;
    QString docTitle;
    QStringList stack; // titles of the enclosing sections, index: level-minLevel
    for (int h=0; h<headings.size(); ++h)
    {
        const auto& hd=headings[h];
        if (hd.level<minLevel)
        {
            stack.clear();
            if (hd.level==1)
                docTitle=hd.title;
            continue;
        }
        if (hd.level>maxLevel)
            continue;

        while (stack.size()>hd.level-minLevel)
            stack.removeLast();
        while (stack.size()<hd.level-minLevel)
            stack.append(QString()); // skipped level
        stack.append(hd.title);

        int end=lines.size();
        for (int k=h+1; k<headings.size(); ++k)
        {
            if (headings[k].level<=hd.level)
            {
                end=headings[k].line;
                break;
            }
        }

        ReferenceSection s;
        s.title=hd.title;
        s.level=hd.level;
        QStringList p;
        if (!docTitle.isEmpty())
            p<<docTitle;
        for (const auto& t: stack)
            if (!t.isEmpty())
                p<<t;
        s.path=p.join(" > ");
        s.text=lines.mid(hd.line, end-hd.line).join('\n').trimmed();
        sections.append(s);
    }
    return sections;
}




void ReferenceIndex::addDocument(const QString& markdown, const QStringList& excludedTitles)
{
    QStringList excludedPaths;
    for (const auto& s: splitMarkdownSections(markdown))
    {
        bool excluded=false;
        for (const auto& p: excludedPaths)
            if (s.path.startsWith(p+" > "))
                excluded=true;
        if (!excluded && excludedTitles.contains(s.title, Qt::CaseInsensitive))
        {
            excludedPaths<<s.path;
            excluded=true;
        }
        if (!excluded)
            sections_.append(s);
    }
}


QString ReferenceIndex::tableOfContents(int inlineLevel) const
{
    QString toc;
    QString docTitle;
    QStringList inlined;
    auto flushInlined=[&]()
    {
        if (!inlined.isEmpty())
        {
            toc+=": "+inlined.join(", ");
            inlined.clear();
        }
    };

    for (const auto& s: sections_)
    {
        QString dt=s.path.section(" > ", 0, 0);
        if (s.level>=inlineLevel)
        {
            inlined<<s.title;
            continue;
        }
        flushInlined();
        if (dt!=docTitle && s.path.contains(" > "))
        {
            docTitle=dt;
            toc+=QString(toc.isEmpty() ? "" : "\n")+"# "+docTitle;
        }
        toc+="\n"+QString(2*std::max(0, s.level-2), ' ')+"- "+s.title;
    }
    flushInlined();
    return toc.trimmed()+"\n";
}


const ReferenceSection* ReferenceIndex::section(const QString& title) const
{
    for (const auto& s: sections_)
        if (s.title.compare(title, Qt::CaseInsensitive)==0)
            return &s;
    return nullptr;
}


ReferenceIndex::LookupResult ReferenceIndex::lookup(const QString& name) const
{
    LookupResult r;

    QString n=name.trimmed();
    // tolerate a leading "#", "- " or quotes
    while (n.startsWith('#') || n.startsWith('"') || n.startsWith('`') || n.startsWith('-'))
        n=n.mid(1).trimmed();
    while (n.endsWith('"') || n.endsWith('`'))
        n.chop(1);
    n=n.trimmed();
    if (n.isEmpty())
        return r;

    // 1. full path
    for (const auto& s: sections_)
        if (s.path.compare(n, Qt::CaseInsensitive)==0)
        {
            r.sections<<&s;
            return r;
        }

    // 1b. path without document title (may exist in several documents)
    for (const auto& s: sections_)
        if (s.path.section(" > ", 1).compare(n, Qt::CaseInsensitive)==0)
            r.sections<<&s;
    if (!r.sections.isEmpty())
        return r;

    // 2. title: all sections with this title
    for (const auto& s: sections_)
        if (s.title.compare(n, Qt::CaseInsensitive)==0)
            r.sections<<&s;
    if (!r.sections.isEmpty())
        return r;

    // 3. partial match of the title, then of the path
    // (paths contain the titles of the parents: a match there would select all subsections)
    QList<const ReferenceSection*> partial;
    for (const auto& s: sections_)
        if (s.title.contains(n, Qt::CaseInsensitive))
            partial<<&s;
    if (partial.isEmpty())
        for (const auto& s: sections_)
            if (s.path.contains(n, Qt::CaseInsensitive))
                partial<<&s;

    if (partial.size()==1)
        r.sections=partial;
    else
        for (const auto* s: partial)
            r.candidates<<s->path;
    return r;
}


} // namespace assistant
} // namespace insight
