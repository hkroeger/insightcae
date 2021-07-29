#ifndef IQREMOTEEXECUTIONSTATE_H
#define IQREMOTEEXECUTIONSTATE_H

#include <QObject>
#include "base/remoteexecution.h"

class AnalysisForm;

/**
 * @brief The IQRemoteExecutionState class
 * holds the selected remote location.
 * Once remote-based actions are started,
 * it needs to be commited to connect a
 * local directory with the remote location.
 */
class IQRemoteExecutionState
: public QObject/*,
  public insight::RemoteLocation*/
{
  AnalysisForm *af_;

//  std::unique_ptr<insight::RemoteExecutionConfig> rec_;
  std::unique_ptr<insight::RemoteLocation> rlc_;

  void setAFEnabledState(bool enabled);

public:
  /**
   * @brief QRemoteExecutionState
   * set from insight::RemoteLocation object
   * @param af
   * @param remoteLocation
   */
  IQRemoteExecutionState(AnalysisForm *af,
//                        const boost::filesystem::path& location,
                        const insight::RemoteLocation& remoteLocation);

  /**
   * @brief QRemoteExecutionState
   * read from config file
   * @param af
   * @param location
   * @param localREConfigFile
   */
  IQRemoteExecutionState(AnalysisForm *af,
                         const boost::filesystem::path& configFile );

  IQRemoteExecutionState(AnalysisForm *af,
                         insight::RemoteServer::ConfigPtr rsc,
//                         const boost::filesystem::path& location,
                         const boost::filesystem::path& remotePath = ""/*,
                         const boost::filesystem::path& localREConfigFile = ""*/);

  ~IQRemoteExecutionState();

  void commit(const boost::filesystem::path& location);

  insight::RemoteLocation& location() const;
  insight::RemoteExecutionConfig& exeConfig();

  void cleanup(bool forceRemoval=false);
};

#endif // IQREMOTEEXECUTIONSTATE_H
