#ifndef VIEWWIDGETACTION_H
#define VIEWWIDGETACTION_H

#include "toolkit_gui_export.h"


#include "cadtypes.h"
#include "feature.h"

#include <memory>

#include <QWidget>
#include <QRubberBand>

#include <QMetaObject>
#include <QApplication>
#include <qnamespace.h>
#include <typeindex>


#include "iqcadmodel3dviewer/viewwidgetactionhost.h"




class QoccViewWidget;

class ToNotepadEmitter
        : public QObject
{
    Q_OBJECT

public:
    ToNotepadEmitter();
    virtual ~ToNotepadEmitter();

Q_SIGNALS:
    void appendToNotepad(const QString& text);
};





template<class Viewer>
class ViewWidgetAction
  : public ViewWidgetActionHost<Viewer>
{

public:
    /**
     * @brief connectActionIsFinished
     * queue into event loop, don't execute directly
     * since object might delete itself
     * @param f
     */
    void connectActionIsFinished(std::function<void(bool)> f)
    {
        actionIsFinished.connect([f](bool success){
            QMetaObject::invokeMethod(
                qApp,
                std::bind(f, success),
                Qt::QueuedConnection
                );
        });
    }

private:
    boost::signals2::signal<void(bool)> actionIsFinished;


protected:
  virtual void finishAction(bool accepted=true)
  {
      actionIsFinished(accepted); //finished_=true;
  }

public:
  using ViewWidgetActionHost<Viewer>::ViewWidgetActionHost;

  ViewWidgetAction(
      ViewWidgetActionHost<Viewer>& parent,
      bool captureAllInput=true)
    : ViewWidgetActionHost<Viewer>(parent, captureAllInput)
  {}

  bool onKeyPress(Qt::KeyboardModifiers modifiers, int key) override
  {
      if (!this->toFirstChildAction(
          &InputReceiver<Viewer>::onKeyPress,
              modifiers, key))
      {
          if (key == Qt::Key_Escape)
          {
              finishAction();
              return true;
          }
      }
      return this->capturesAllInput_;
  }

  virtual void start() =0;

  virtual QString description() const
  {
      return QString();
  }
};




template<class VWA, typename... Args>
std::unique_ptr<VWA, typename InputReceiver<typename VWA::Viewer_type>::Deleter>
make_viewWidgetAction(Args&&... args)
{
    return std::unique_ptr<VWA, typename InputReceiver<typename VWA::Viewer_type>::Deleter >
        (new VWA(std::forward<Args>(args)...));
}





#endif // VIEWWIDGETACTION_H
