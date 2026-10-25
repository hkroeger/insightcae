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

#include "iqiscadsyntaxhighlighter.h"

#include <QTextBlock>

#include <algorithm>
#include <functional>


using namespace insight::cad::parser;


namespace {


enum class LexClass { Comment, String, Number, Identifier, AtKeyword };


inline bool isIdentifierStart(QChar c)
{
    return c.isLetter() || c==QChar('_');
}

inline bool isIdentifierChar(QChar c)
{
    return c.isLetterOrNumber() || c==QChar('_');
}


/**
 * split a line into lexical elements (consistent with
 * the skip grammar of the ISCAD parser).
 * @return the block state at the end of the line
 */
int scanLine(
    const QString& text, int previousBlockState,
    const std::function<void(int begin, int length, LexClass)>& found )
{
    const int n=text.size();
    int i=0;

    auto at = [&](int j) { return j<n ? text[j] : QChar(); };

    if (previousBlockState==IQISCADSyntaxHighlighter::InBlockComment)
    {
        int e=text.indexOf("*/");
        if (e<0)
        {
            found(0, n, LexClass::Comment);
            return IQISCADSyntaxHighlighter::InBlockComment;
        }
        found(0, e+2, LexClass::Comment);
        i=e+2;
    }

    while (i<n)
    {
        QChar c=text[i];

        if ( c==QChar('#') || (c==QChar('/') && at(i+1)==QChar('/')) )
        {
            found(i, n-i, LexClass::Comment);
            return IQISCADSyntaxHighlighter::Normal;
        }
        else if (c==QChar('/') && at(i+1)==QChar('*'))
        {
            int e=text.indexOf("*/", i+2);
            if (e<0)
            {
                found(i, n-i, LexClass::Comment);
                return IQISCADSyntaxHighlighter::InBlockComment;
            }
            found(i, e+2-i, LexClass::Comment);
            i=e+2;
        }
        else if (c==QChar('\'') || c==QChar('"'))
        {
            int e=text.indexOf(c, i+1);
            if (e<0) e=n-1;
            found(i, e+1-i, LexClass::String);
            i=e+1;
        }
        else if ( c.isDigit() || (c==QChar('.') && at(i+1).isDigit()) )
        {
            int j=i;
            while (j<n && text[j].isDigit()) ++j;
            if (at(j)==QChar('.'))
            {
                ++j;
                while (j<n && text[j].isDigit()) ++j;
            }
            if ( (at(j)==QChar('e') || at(j)==QChar('E'))
                 && ( at(j+1).isDigit()
                      || ( (at(j+1)==QChar('+')||at(j+1)==QChar('-')) && at(j+2).isDigit() ) ) )
            {
                j+=2;
                while (j<n && text[j].isDigit()) ++j;
            }
            found(i, j-i, LexClass::Number);
            i=j;
        }
        else if (isIdentifierStart(c) || (c==QChar('@') && isIdentifierStart(at(i+1))))
        {
            int j=i+1;
            while (j<n && isIdentifierChar(text[j])) ++j;
            found(i, j-i, c==QChar('@') ? LexClass::AtKeyword : LexClass::Identifier);
            i=j;
        }
        else
        {
            ++i;
        }
    }

    return IQISCADSyntaxHighlighter::Normal;
}


}




IQISCADSyntaxHighlighter::IQISCADSyntaxHighlighter(QTextDocument* parent)
: QSyntaxHighlighter(parent),
  errorBegin_(-1), errorEnd_(-1)
{
    auto fmt = [](const QColor& c, bool bold=false, bool italic=false)
    {
        QTextCharFormat f;
        f.setForeground(c);
        if (bold) f.setFontWeight(QFont::Bold);
        if (italic) f.setFontItalic(true);
        return f;
    };

    std::vector<TokenKind> definitions = {
        TokenKind::FeatureDef, TokenKind::ComponentDef, TokenKind::ScalarDef,
        TokenKind::VectorDef, TokenKind::DatumDef, TokenKind::SelectionDef };
    std::vector<TokenKind> others = {
        TokenKind::FeatureCommand, TokenKind::ScalarFunction,
        TokenKind::VectorFunction, TokenKind::PostprocCommand,
        TokenKind::FeatureRef, TokenKind::ScalarRef, TokenKind::VectorRef,
        TokenKind::DatumRef, TokenKind::SelectionRef };

    for (auto k: definitions)
    {
        tokenFormats_[k]=fmt(tokenColor(k), true);
    }
    tokenFormats_[TokenKind::ComponentDef].setFontUnderline(true);

    for (auto k: others)
    {
        bool isCommand =
            k==TokenKind::FeatureCommand || k==TokenKind::ScalarFunction
            || k==TokenKind::VectorFunction || k==TokenKind::PostprocCommand;
        tokenFormats_[k]=fmt(tokenColor(k), isCommand);
    }

    commentFormat_ = fmt(Qt::gray, false, true);
    stringFormat_ = fmt(QColor(170, 85, 0));
    numberFormat_ = fmt(QColor(0, 0, 200));
    keywordFormat_ = fmt(QColor(128, 0, 128), true);
    callFormat_ = fmt(tokenColor(TokenKind::FeatureCommand), true);
    definitionFormat_ = fmt(tokenColor(TokenKind::FeatureDef), true);

    // keywords of the ISCAD grammar (kw(...) in the parser rules).
    // Single letter keywords are left out, they are likely symbol names.
    for (const char* kw: {
         "BOMDescription", "PiM", "Plane", "RefAxis", "RefPt", "SPlane", "TPlane",
         "aligned", "alledges", "allfaces", "allsolids", "allvertices", "areaWeight",
         "around", "as", "ascii", "assembly", "at", "axial", "baseFaceGroups", "by",
         "center", "centered", "clipte", "coaxial", "cost", "density", "der",
         "diameter", "dir", "direction", "dx", "edge", "edgeGroups", "edgeRefinements",
         "edges", "eid", "end", "ext", "ext0", "ext1", "face", "faceGroups", "faces",
         "fid", "fixedbinormal", "from", "hmiter", "in", "inc", "inclined", "inplane",
         "inside", "inverted", "keepTmpDir", "layer", "length", "linear", "meshSizes",
         "nLayers", "nearest", "not", "onaxis", "orient", "oriented", "outside",
         "parallel", "planeparallel", "points", "poly", "r_AK", "r_EK", "radial",
         "reapprox", "resolution", "screwBases", "screwHeads", "screws", "section",
         "sid", "skiphl", "solid", "solids", "start", "sub", "to", "topFaceGroups",
         "up", "vertex", "vertexGroups", "vertices", "vid", "visresolution", "vmiter",
         "volumeGroups", "xsec_axpl", "xsec_plpl", "xsec_ppp" } )
    {
        keywords_.insert(kw);
    }
}


QColor IQISCADSyntaxHighlighter::tokenColor(TokenKind kind)
{
    switch (kind)
    {
        case TokenKind::FeatureCommand:
            return QColor(0, 0, 139);
        case TokenKind::ScalarFunction:
        case TokenKind::VectorFunction:
            return QColor(0, 110, 110);
        case TokenKind::PostprocCommand:
            return QColor(139, 0, 139);
        case TokenKind::FeatureDef:
        case TokenKind::ComponentDef:
        case TokenKind::FeatureRef:
            return QColor(139, 0, 0);
        case TokenKind::ScalarDef:
        case TokenKind::ScalarRef:
        case TokenKind::VectorDef:
        case TokenKind::VectorRef:
            return QColor(0, 120, 0);
        case TokenKind::DatumDef:
        case TokenKind::DatumRef:
            return QColor(160, 90, 0);
        case TokenKind::SelectionDef:
        case TokenKind::SelectionRef:
            return QColor(90, 60, 170);
    }
    return Qt::black;
}


bool IQISCADSyntaxHighlighter::isInCommentOrString(
    const QString &lineText, int pos, int previousBlockState )
{
    bool inside=false;
    scanLine(
        lineText, previousBlockState,
        [&](int b, int l, LexClass c)
        {
            if (c==LexClass::Comment && b<pos && pos<=b+l) inside=true;
            if (c==LexClass::String && b<pos && pos<b+l) inside=true;
        } );
    return inside;
}


void IQISCADSyntaxHighlighter::setTokens(std::vector<IQISCADToken>&& tokens)
{
    tokens_=std::move(tokens);
}


void IQISCADSyntaxHighlighter::setErrorRange(int begin, int end)
{
    errorBegin_=begin;
    errorEnd_=std::max(begin+1, end);
}


void IQISCADSyntaxHighlighter::adaptToContentsChange(int pos, int removed, int added)
{
    const int delta=added-removed;
    const auto* doc=document();

    // without semantic information, the reformatting by QSyntaxHighlighter is correct
    const bool hadAnnotations = !tokens_.empty() || errorBegin_>=0;

    auto isIdentifierCharAt = [doc](int p)
    {
        return p>=0 && p<doc->characterCount() && isIdentifierChar(doc->characterAt(p));
    };

    std::vector<IQISCADToken> adapted;
    adapted.reserve(tokens_.size());
    for (const auto& t: tokens_)
    {
        if (t.end<pos || (t.end==pos && !isIdentifierCharAt(pos)))
        {
            // before the change
            adapted.push_back(t);
        }
        else if ( t.begin>pos+removed
                 || (t.begin==pos+removed && !isIdentifierCharAt(pos+added-1)) )
        {
            // after the change
            adapted.push_back(IQISCADToken{t.begin+delta, t.end+delta, t.kind});
        }
        // otherwise the token was edited: remove until the next parse result arrives
    }
    tokens_=std::move(adapted);

    if (errorBegin_>=0)
    {
        if (errorBegin_>=pos+removed)
        {
            errorBegin_+=delta;
            errorEnd_+=delta;
        }
        else if (errorEnd_>=pos)
        {
            // the erroneous text is being edited
            errorBegin_=errorEnd_=-1;
        }
    }

    if (!hadAnnotations) return;

    // QSyntaxHighlighter has already reformatted the changed blocks,
    // but with the previous token positions. Redo.
    auto b=doc->findBlock(pos);
    auto e=doc->findBlock(pos+added);
    while (b.isValid())
    {
        rehighlightBlock(b);
        if (b==e) break;
        b=b.next();
    }
}


void IQISCADSyntaxHighlighter::highlightBlock(const QString& text)
{
    highlightLexical(text);
    highlightTokens(text);
    highlightError(text);
}


void IQISCADSyntaxHighlighter::highlightLexical(const QString &text)
{
    // position of the first non-blank character
    int firstNonBlank=0;
    while (firstNonBlank<text.size() && text[firstNonBlank].isSpace())
        ++firstNonBlank;

    auto nextNonBlank = [&text](int j)
    {
        while (j<text.size() && text[j].isSpace()) ++j;
        return j<text.size() ? text[j] : QChar();
    };

    int state=scanLine(
        text, previousBlockState(),
        [&](int b, int l, LexClass c)
        {
            switch (c)
            {
                case LexClass::Comment:
                    setFormat(b, l, commentFormat_);
                    break;
                case LexClass::String:
                    setFormat(b, l, stringFormat_);
                    break;
                case LexClass::Number:
                    setFormat(b, l, numberFormat_);
                    break;
                case LexClass::AtKeyword:
                    setFormat(b, l, keywordFormat_);
                    break;
                case LexClass::Identifier:
                {
                    QChar nx=nextNonBlank(b+l);
                    if (keywords_.contains(text.mid(b, l)))
                    {
                        setFormat(b, l, keywordFormat_);
                    }
                    else if (nx==QChar('('))
                    {
                        // preliminary, until the parser has recognized the command
                        setFormat(b, l, callFormat_);
                    }
                    else if ( b==firstNonBlank
                              && (nx==QChar(':') || nx==QChar('=') || nx==QChar('?')) )
                    {
                        // preliminary, assignment at the beginning of a line
                        setFormat(b, l, definitionFormat_);
                    }
                    break;
                }
            }
        } );

    setCurrentBlockState(state);
}


void IQISCADSyntaxHighlighter::highlightTokens(const QString &text)
{
    const int blockBegin=currentBlock().position();
    const int blockEnd=blockBegin+text.size();

    // tokens are sorted and do not overlap, i.e. the ends are sorted as well
    auto it=std::partition_point(
        tokens_.begin(), tokens_.end(),
        [blockBegin](const IQISCADToken& t) { return t.end<=blockBegin; } );

    for (; it!=tokens_.end() && it->begin<blockEnd; ++it)
    {
        int b=std::max(it->begin, blockBegin);
        int e=std::min(it->end, blockEnd);
        auto f=tokenFormats_.find(it->kind);
        if (e>b && f!=tokenFormats_.end())
        {
            setFormat(b-blockBegin, e-b, f->second);
        }
    }
}


void IQISCADSyntaxHighlighter::highlightError(const QString &text)
{
    if (errorBegin_<0) return;

    const int blockBegin=currentBlock().position();
    // include the line end, so that an error at the end of a line is visible
    const int blockEnd=blockBegin+text.size()+1;

    int b=std::max(errorBegin_, blockBegin);
    int e=std::min(errorEnd_, blockEnd);
    if (b>=e) return;

    // an error at the line end is marked at the last character
    if (b-blockBegin>=text.size())
    {
        if (text.isEmpty()) return;
        b=blockBegin+text.size()-1;
    }
    e=std::min(e, blockBegin+int(text.size()));

    for (int i=b-blockBegin; i<e-blockBegin; ++i)
    {
        auto f=format(i);
        f.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        f.setUnderlineColor(Qt::red);
        setFormat(i, 1, f);
    }
}
