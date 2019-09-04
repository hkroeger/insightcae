project(InsightConfiguration)

install_script(insight_setenv insight_setenv.sh)
install_script(insight_aliases insight_aliases.sh)

if (INSIGHT_INSTALL_DESKTOP)
 install(FILES insight_profile.sh DESTINATION /etc/profile.d)
 # hack: otherwise defined alias'es might not survive...
 install(FILES insight_aliases.sh DESTINATION /etc/bash_completion.d)
endif()
