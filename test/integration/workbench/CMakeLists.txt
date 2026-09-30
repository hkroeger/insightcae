# RemoteRun of the workbench, driven through an offscreen AnalysisForm
remote_backend_test(remote_workbench_remoterun
    LIBS workbench_core dummiesgui
    LABELS integration workbench
    TIMEOUT 1800)
