#ifndef IQREMOTEEXECUTIONSTATE_H
#define IQREMOTEEXECUTIONSTATE_H

#include "toolkit_gui_export.h"

#include <QObject>
#include "base/remoteexecution.h"
#include "base/cppextensions.h"
#include "base/tools.h"


class AnalysisForm;


/**
 * @brief The IQRemoteExecutionState class
 * holds the selected remote location.
 * Once remote-based actions are started,
 * it needs to be commited to connect a
 * local directory with the remote location.
 */
class TOOLKIT_GUI_EXPORT IQRemoteExecutionState
: public QObject
{

protected:
  virtual void updateGUI(bool enabled);

  std::unique_ptr<insight::RemoteLocation> rlc_;
  IQRemoteExecutionState();

public:
  /**
   * @brief QRemoteExecutionState
   * set from insight::RemoteLocation object
   * @param af
   * @param remoteLocation
   */
  template<typename DerivedClass, class ...Args>
  static IQRemoteExecutionState* New(
      QObject *parent,
      Args&&... addArgs )
  {
    auto *iqr = new insight::DestructionGuard<DerivedClass>;
    iqr->setParent(parent);
    iqr->rlc_ = std::make_unique<insight::RemoteLocation>(
                std::forward<Args>(addArgs)... );
    dynamic_cast<IQRemoteExecutionState*>(iqr)->updateGUI(true);
    iqr->setPreDestructionFunction(
                std::bind(&IQRemoteExecutionState::updateGUI, iqr, false)
                );
    return iqr;
  }

  bool remoteHostRunningAndDirectoryExisting() const;
  void commit(const boost::filesystem::path& location);
  bool isCommitted() const;

  insight::RemoteLocation& location() const;
  insight::RemoteExecutionConfig& exeConfig();

  virtual void cleanup(bool forceRemoval=false);

};





#endif // IQREMOTEEXECUTIONSTATE_H
