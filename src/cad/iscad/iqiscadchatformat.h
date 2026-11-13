#ifndef IQISCADCHATFORMAT_H
#define IQISCADCHATFORMAT_H

#include <QColor>
#include <QFont>
#include <QString>
#include <QTextDocumentFragment>

class QTextDocument;


/**
 * formatting helpers of the assistant chat transcript
 */
namespace IQISCADChatFormat
{

/**
 * renders markdown into a fragment with the text color color
 * and all font sizes scaled by scale (relative to baseFont; headings stay proportionally larger).
 * Used to show the system prompt subdued in the transcript.
 */
QTextDocumentFragment subduedMarkdown(
    const QString& markdown,
    const QFont& baseFont,
    const QColor& color,
    double scale = 0.8 );

/**
 * replaces the paragraph, which contains only the text marker, by the fragment.
 * Returns false, if the marker was not found.
 */
bool replaceMarker(QTextDocument* doc, const QString& marker, const QTextDocumentFragment& fragment);

/**
 * a color between text and background (for subdued text in light and dark themes)
 */
QColor subduedColor(const QColor& text, const QColor& background, double weight = 0.55);

}

#endif // IQISCADCHATFORMAT_H
