
# detect the system and version
# like ubuntu-focal or centos-7
# https://unix.stackexchange.com/a/6348
if type lsb_release >/dev/null 2>&1; then
    OS=$(lsb_release -si| awk '{print tolower($0)}') # ubuntu
    VER=$(lsb_release -sc) # jammy
elif [ -f /etc/debian_version ]; then
    # Older Debian/Ubuntu/etc.
    OS=debian
    VER=$(cat /etc/debian_version)
elif [ -f /etc/redhat-release ]; then
    OS=$(cat /etc/redhat-release |cut -d ' ' -f 1| awk '{print tolower($0)}')
    VER=$(cat /etc/redhat-release |cut -d ' ' -f 4|cut -d '.' -f 1)
elif [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$(echo $NAME|(read OS REST; echo $OS))
    VER=${VERSION%.*}
fi
SYSTEM=${OS}-${VER}

CURRENT_BRANCH=$(git log -n 1 --pretty=%d HEAD|sed -e 's# ([^,]*, .*/\(.*\))#\1#g')

source $SCRIPTPATH/configuration.sh
