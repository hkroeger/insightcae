#ifndef INSIGHT_QACTIONPROGRESSDISPLAYERWIDGET_H
#define INSIGHT_QACTIONPROGRESSDISPLAYERWIDGET_H

#include "toolkit_gui_export.h"


#include <QWidget>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "base/progressdisplayer.h"


namespace insight {

class TOOLKIT_GUI_EXPORT QActionProgressDisplayerWidget
: public QWidget,
  public insight::ProgressDisplayer
{

protected:
    struct ProgressItem
    {
      QLabel* lbl;
      QProgressBar* p;
      inline void deleteLater()
      {
        p->setValue(p->maximum());
        p->deleteLater();
        lbl->deleteLater();
      }
    };

    struct Rows
    {
      QVBoxLayout* vlayout;
      QLabel* lbl;
      std::map<std::string,ProgressItem> items;
      inline void deleteLater()
      {
        lbl->deleteLater();
        for (auto& i: items)
          i.second.deleteLater();
        items.clear();
      }
    };
    typedef std::map<std::string,Rows> Columns;

    QHBoxLayout *hlayout_;
    Columns columns_;

    Columns::iterator getColumn(const std::string& path, std::vector<std::string>& splitPath);
    ProgressItem getOrCreateItem(const std::string& path, bool forbidCreation=false);
    void deleteItem(const std::string& path);


public:
    QActionProgressDisplayerWidget(QWidget* parent=nullptr);

    void update ( const ProgressState& pi ) override;
    void logMessage(const std::string& line) override;
    void setActionProgressValue(const std::string& path, double value) override;
    void setMessageText(const std::string& path, const std::string& message) override;
    void finishActionProgress(const std::string& path) override;
    void reset() override;
};

} // namespace insight

#endif // INSIGHT_QPROGRESSDISPLAYERWIDGET_H
