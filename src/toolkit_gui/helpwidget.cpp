#include "helpwidget.h"
#include <QDebug>

HelpWidget::HelpWidget(QWidget* p, const insight::SimpleLatex& content)
: QTextEdit(p),
  content_(content),
  cur_content_width_(-1)
{
//  setHtml( content_.toHTML(width()).c_str() );
}


void HelpWidget::setContent(const insight::SimpleLatex& content)
{
  content_=content;
}

void HelpWidget::resizeEvent(QResizeEvent *event)
{
  QTextEdit::resizeEvent(event);
  auto newwidth= viewport()->width(); //width() - contentsMargins().left() - contentsMargins().right();
  if (newwidth!=cur_content_width_)
  {
    cur_content_width_=newwidth; // width();
    setHtml( content_.toHTML(cur_content_width_).c_str() );
  }
}
