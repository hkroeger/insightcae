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

class ISCADSyntaxHighlighter
: public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ISCADSyntaxHighlighter(QTextDocument *parent = 0);

    void setHighlightWord(const QString& word);
    
protected:
    void highlightBlock(const QString &text);
    

private:
  enum HighlightingRule_Index
  {
    HighlightingRule_Function,
    HighlightingRule_ModelStepDef,
    HighlightingRule_CommentHash,
    HighlightingRule_SelectedKeyword,
    HighlightingRule_Index_Max
  };
  
  struct HighlightingRule
  {
      QRegExp pattern;
      QTextCharFormat format;
  };
  
  QVector<HighlightingRule> highlightingRules;
};


#endif // ISCADSYNTAXHIGHLIGHTER_H
