#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QTextEdit>
#include "base/latextools.h"

class HelpWidget
: public QTextEdit
{
  insight::SimpleLatex content_;
  int cur_content_width_;

public:
  HelpWidget(QWidget* parent, const insight::SimpleLatex& content);
  void setContent(const insight::SimpleLatex& content);

  void	resizeEvent(QResizeEvent *event) override;
};

#endif // HELPWIDGET_H
