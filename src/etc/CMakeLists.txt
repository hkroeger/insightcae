project(InsightConfiguration)

install_script(insight_setenv insight_setenv.sh)

if (INSIGHT_INSTALL_DESKTOP)
 # hack: otherwise defined alias'es might not survive...
 install(FILES insight_profile.sh DESTINATION /etc/bash_completion.d)
endif()
