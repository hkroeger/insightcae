#ifndef INSIGHT_ASSISTANT_REFERENCEDOCS_H
#define INSIGHT_ASSISTANT_REFERENCEDOCS_H

#include <QString>
#include <QStringList>
#include <QList>

namespace insight {
namespace assistant {


/**
 * a section of a markdown document
 */
struct ReferenceSection
{
    QString title;

    /**
     * heading level (number of #)
     */
    int level = 0;

    /**
     * titles of the document (level 1 heading), the enclosing sections and of this one, joined by " > "
     */
    QString path;

    /**
     * the section including its heading and all subsections
     */
    QString text;
};


/**
 * splits a markdown document at its headings of levels minLevel to maxLevel.
 * Lines in fenced code blocks are not taken as headings.
 */
QList<ReferenceSection> splitMarkdownSections(const QString& markdown, int minLevel=2, int maxLevel=4);


/**
 * sections of the language documentation, which the assistant can read on demand
 */
class ReferenceIndex
{
    QList<ReferenceSection> sections_;

public:
    /**
     * add the sections of a document, except those with the given titles (and their subsections)
     */
    void addDocument(const QString& markdown, const QStringList& excludedTitles=QStringList());

    inline const QList<ReferenceSection>& sections() const { return sections_; }
    inline bool isEmpty() const { return sections_.isEmpty(); }

    /**
     * the headings as indented list. Headings of level inlineLevel and deeper
     * are listed in one line after their parent (e.g. the command names).
     */
    QString tableOfContents(int inlineLevel=4) const;

    struct LookupResult
    {
        /**
         * the found sections (several, if the title occurs in several documents)
         */
        QList<const ReferenceSection*> sections;

        /**
         * paths of the matching sections, if a partial match is not unique
         */
        QStringList candidates;
    };

    /**
     * find sections: by full path, by title (case insensitive, all matches),
     * then by a unique partial match of the title or path.
     */
    LookupResult lookup(const QString& name) const;

    /**
     * the section with this title (case insensitive) or nullptr
     */
    const ReferenceSection* section(const QString& title) const;
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_REFERENCEDOCS_H
