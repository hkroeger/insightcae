project(InsightConfiguration)

install_script(insight_setenv insight_setenv.sh)
install_shared_file(testcases_module_cfg testcases.module modules.d)
install_shared_file(genericmodules_module_cfg genericmodules.module modules.d)

install(FILES insight_profile.sh DESTINATION /etc/profile.d)
