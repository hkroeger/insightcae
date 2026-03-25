#include "iqbackgroundtask.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include <exception>

#include <QApplication>

using namespace insight;


IQBackgroundTask::IQBackgroundTask(
    const std::string& name,
    insight::ProgressDisplayer *pd )
    : name_(name), pd_(pd)
{}

void IQBackgroundTask::start(
    std::function<void(insight::ActionProgress&)> action,
    bool installDefaultFailureSignalReceiver
    )
{

    if (installDefaultFailureSignalReceiver)
    {
        QObject::connect(
            this, &IQBackgroundTask::failed, qApp,
            [](std::exception_ptr e) { std::rethrow_exception(e); }
            );
    }

    launch(
          [this,action]()
          {
              auto *tpd=pd_;
              if (!tpd)
                  tpd=&consoleProgressDisplayer;

              {
                  auto ap=tpd->forkNewAction(99, name_);
                  ap->stopSignal().connect(
                    [this]()
                    {
                          interrupt();
                    });
                  action(*ap);
              }

              this->deleteLater();
              Q_EMIT finished();
          },

          std::function{[this](::std::exception_ptr e)  // exception handler
          {
              try
              {
                  if (e) std::rethrow_exception(e);
              }
              catch (...)
              {
                  Q_EMIT failed(e);
                  this->deleteLater();
              }
          }},

          std::function{[this]() // interrupt handler
          {
              Q_EMIT cancelled();
              this->deleteLater();
          }}
        );
}
