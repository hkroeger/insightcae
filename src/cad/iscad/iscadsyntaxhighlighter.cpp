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

#include "iscadsyntaxhighlighter.h"


  
  
ISCADSyntaxHighlighter::ISCADSyntaxHighlighter(QTextDocument* parent)
: QSyntaxHighlighter(parent)
{
  highlightingRules.resize(HighlightingRule_Index_Max);
  
  QString ident_pat("[a-zA-Z][a-zA-Z0-9_]*");
  
  {
    HighlightingRule rule;
    rule.pattern=QRegExp();
    rule.format.setForeground(Qt::darkBlue);
    rule.format.setBackground(Qt::yellow);
    rule.format.setFontWeight(QFont::Bold);
    
    highlightingRules[HighlightingRule_SelectedKeyword]=rule;
  }

  {
    HighlightingRule rule;
    rule.pattern=QRegExp("(#.*)$");
    rule.format.setForeground(Qt::gray);    
    rule.format.setBackground(Qt::white);
    rule.format.setFontWeight(QFont::Normal);
    highlightingRules[HighlightingRule_CommentHash]=rule;
  }

  {
    HighlightingRule rule;
    rule.pattern=QRegExp("\\b("+ident_pat+")\\b *\\(");
    rule.format.setForeground(Qt::darkBlue);
    rule.format.setBackground(Qt::white);
    rule.format.setFontWeight(QFont::Bold);
    
    highlightingRules[HighlightingRule_Function]=rule;
  }
  
  {
    HighlightingRule rule;
    rule.pattern=QRegExp("\\b("+ident_pat+")\\b *\\:");
    rule.format.setForeground(Qt::darkRed);
    rule.format.setBackground(Qt::white);
    rule.format.setFontWeight(QFont::Bold);
    
    highlightingRules[HighlightingRule_ModelStepDef]=rule;
  }
}

void ISCADSyntaxHighlighter::setHighlightWord(const QString& word)
{
//   qDebug()<<"setting highlight word = "<<word<<endl;
  
  if (word.isEmpty())
  {
    highlightingRules[HighlightingRule_SelectedKeyword].pattern=QRegExp();
  }
  else
  { 
    highlightingRules[HighlightingRule_SelectedKeyword].pattern=QRegExp("\\b("+word+")\\b");
  }
}



void ISCADSyntaxHighlighter::highlightBlock(const QString& text)
{
  foreach (const HighlightingRule &rule, highlightingRules) 
  {
    if (!rule.pattern.isEmpty())
    {
      QRegExp expression(rule.pattern);
      
      int index0 = expression.indexIn(text);
      int index = expression.pos(1);
      
      while (index0 >= 0) 
      {
 	int fulllength = expression.matchedLength();
	int length = expression.cap(1).length();
	setFormat(index0, length, rule.format);
	
	index0 = expression.indexIn(text, index0 + fulllength);
	index = expression.pos(1);
      }
    }
  }
}
