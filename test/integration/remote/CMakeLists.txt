# backend-parametrised tests: each one is registered for the backends
# localshell, ssh-localhost (fixture remote_ssh), ssh-env and wsl-env (opt-in)
remote_backend_test(remote_server_fileops      LABELS integration)
remote_backend_test(remote_server_backgroundjob LABELS integration)
remote_backend_test(remote_server_sync         LABELS integration)
remote_backend_test(remote_location            LABELS integration)
remote_backend_test(remote_analyze_e2e         LABELS integration TIMEOUT 1200)

# SSH specific
remote_backend_test(remote_ssh_portmapping     LABELS integration SSH_ONLY)
remote_backend_test(remote_ssh_lifecycle       LABELS integration SSH_ONLY)
