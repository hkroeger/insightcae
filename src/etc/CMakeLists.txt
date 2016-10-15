project(InsightConfiguration)

install_script(insight_setenv insight_setenv.sh)

install(FILES insight_profile.sh DESTINATION /etc/profile.d)
