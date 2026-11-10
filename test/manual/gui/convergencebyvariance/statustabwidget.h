#ifndef STATUSTABWIDGET_H
#define STATUSTABWIDGET_H

#include <QTabBar>
#include <QTabWidget>
#include <QColor>

#include <vector>

/**
 * Tab bar, which fills each tab with a status color
 * (independent of the widget style)
 */
class StatusTabBar : public QTabBar
{
  Q_OBJECT

  std::vector<QColor> colors_;

public:
  StatusTabBar(QWidget* parent=nullptr);
  void setTabStatusColor(int index, const QColor& c);

protected:
  void paintEvent(QPaintEvent*) override;
};


class StatusTabWidget : public QTabWidget
{
  Q_OBJECT

  StatusTabBar* bar_;

public:
  StatusTabWidget(QWidget* parent=nullptr);
  void setTabStatusColor(int index, const QColor& c);
};

#endif
