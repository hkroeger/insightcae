/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ISCADSYNTAXHIGHLIGHTER_H
#define ISCADSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QSet>

#include <map>
#include <vector>

#include "parser_tokens.h"


/**
 * a token recognized by the parser, in editor (character) positions
 */
struct IQISCADToken
{
    int begin, end;
    insight::cad::parser::TokenKind kind;
};


/**
 * Two-layer syntax highlighting:
 *
 * - lexical: comments, strings, numbers and keywords. Done immediately
 *   for every changed line.
 *
 * - semantic: the tokens, which the background parser has recognized
 *   (commands, symbol definitions and references of different types).
 *   Applied, when the parse result arrives. Until then, the positions
 *   of the previous tokens are adapted to the edits.
 */
class IQISCADSyntaxHighlighter
: public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum BlockState { Normal = 0, InBlockComment = 1 };

    IQISCADSyntaxHighlighter(QTextDocument *parent = 0);

    /**
     * replace the semantic tokens.
     * @param tokens sorted by begin position, non-overlapping
     */
    void setTokens(std::vector<IQISCADToken>&& tokens);
    inline const std::vector<IQISCADToken>& tokens() const { return tokens_; }

    /**
     * mark the given range as erroneous. Pass begin<0 to remove the mark.
     */
    void setErrorRange(int begin, int end);

    /**
     * the document was changed at pos: adapt the token positions.
     * Tokens which overlap the changed range are removed.
     * Has to be called before the changed text is rehighlighted.
     */
    void adaptToContentsChange(int pos, int removed, int added);

    /**
     * the color, which is used for symbols of the given kind
     */
    static QColor tokenColor(insight::cad::parser::TokenKind kind);

    /**
     * determine, if the position inside the given line is within a comment or string
     */
    static bool isInCommentOrString(const QString& lineText, int pos, int previousBlockState);

protected:
    void highlightBlock(const QString &text) override;

private:
    std::map<insight::cad::parser::TokenKind, QTextCharFormat> tokenFormats_;
    QTextCharFormat
        commentFormat_, stringFormat_, numberFormat_,
        keywordFormat_, callFormat_, definitionFormat_;
    QSet<QString> keywords_;

    std::vector<IQISCADToken> tokens_;
    int errorBegin_, errorEnd_;

    /**
     * the lexical layer, also provides preliminary highlighting
     * of command calls and definitions until the parse result arrives
     */
    void highlightLexical(const QString &text);
    void highlightTokens(const QString &text);
    void highlightError(const QString &text);
};


#endif // ISCADSYNTAXHIGHLIGHTER_H
