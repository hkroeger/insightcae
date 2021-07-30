#ifndef IQREMOTEEXECUTIONSTATE_H
#define IQREMOTEEXECUTIONSTATE_H

#include "toolkit_gui_export.h"

#include <QObject>
#include "base/remoteexecution.h"

class AnalysisForm;

class IQRemoteExecutionState;

template <typename T>
class DestructionGuard : public T
{
  friend class IQRemoteExecutionState;
public:
  ~DestructionGuard();
};

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
  template<typename T> friend class DestructionGuard;

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
  template<typename DerivedClass>
  static IQRemoteExecutionState* New(
      QObject *parent,
      const insight::RemoteLocation& remoteLocation )
  {
    auto *iqr = new DestructionGuard<DerivedClass>;
    iqr->setParent(parent);
    iqr->rlc_.reset(new insight::RemoteLocation(remoteLocation));
    iqr->updateGUI(true);
    return iqr;
  }

  /**
   * @brief QRemoteExecutionState
   * read from config file
   * @param af
   * @param location
   * @param localREConfigFile
   */
  template<typename DerivedClass>
  static IQRemoteExecutionState* New(
      QObject *parent,
      const boost::filesystem::path& configFile )
  {
    auto *iqr = new DestructionGuard<DerivedClass>;
    iqr->setParent(parent);
    iqr->rlc_.reset(new insight::RemoteLocation(configFile));
    iqr->updateGUI(true);
    return iqr;
  }

  template<typename DerivedClass>
  static IQRemoteExecutionState* New(
      QObject *parent,
      insight::RemoteServer::ConfigPtr rsc,
      const boost::filesystem::path& remotePath = "" )
  {
    auto *iqr = new DestructionGuard<DerivedClass>;
    iqr->setParent(parent);
    iqr->rlc_.reset(new insight::RemoteLocation(rsc, remotePath));
    iqr->updateGUI(true);
    return iqr;
  }

//  virtual ~IQRemoteExecutionState();

  void commit(const boost::filesystem::path& location);
  bool isCommitted() const;

  insight::RemoteLocation& location() const;
  insight::RemoteExecutionConfig& exeConfig();

  virtual void cleanup(bool forceRemoval=false);

};



template <typename T>
DestructionGuard<T>::~DestructionGuard()
{
  this->updateGUI(false);
}


#endif // IQREMOTEEXECUTIONSTATE_H
