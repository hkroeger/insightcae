
#include "remotesync.h"


namespace insight {


RunSyncToRemote::RunSyncToRemote(insight::RemoteExecutionConfig& rec, bool includeProcDirs)
  : rec_(rec), includeProcDirs_(includeProcDirs)
{}

void RunSyncToRemote::start()
{
    boost::thread::operator=(
        boost::thread(
            [this]() {
                try
                {
                    rec_.syncToRemote
                    (
                        includeProcDirs_,
                        {},
                        [&](int progress, const std::string& progress_text)
                        {
                            Q_EMIT progressValueChanged(progress);
                            Q_EMIT progressTextChanged(QString(progress_text.c_str()));
                        }
                    );
                }
                catch (const boost::thread_interrupted& i)
                {}
                Q_EMIT transferFinished();
            }
            )
        );
}

void RunSyncToRemote::wait()
{
    join();
}




RunSyncToLocal::RunSyncToLocal(insight::RemoteExecutionConfig& rec, bool includeProcDirs)
  : rec_(rec), includeProcDirs_(includeProcDirs)
{}

void RunSyncToLocal::start()
{
    boost::thread::operator=(
        boost::thread(
            [this]() {
                try
                {
                    rec_.syncToLocal
                    (
                        includeProcDirs_,
                        false,
                        {},
                        [&](int progress, const std::string& progress_text)
                        {
                            Q_EMIT progressValueChanged(progress);
                            Q_EMIT progressTextChanged(QString(progress_text.c_str()));
                        }
                    );
                }
                catch (const boost::thread_interrupted& i)
                {}
                Q_EMIT transferFinished();
            }
            )
        );
}

void RunSyncToLocal::wait()
{
    join();
}

}
