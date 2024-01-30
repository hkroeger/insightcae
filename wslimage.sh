#!/bin/bash

##
## This script runs in the superbuild root directory
##

set -e

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/configuration.sh

BUILD_PATH=$SCRIPTPATH/insight-build

usage() {
    echo "Usage: $0 [-h] [-s <directory>] [-d <DEB file>] [-o <file name>]" 1>&2
    echo " -h   help"
    echo " -d   DEB file with InsightCAE to be included into the WSL image"
    echo " -s   Path to build directory, which contains the DEB file"
    echo " -o   output image file name"
    exit 1
}

while getopts "s:d:o:h" o; do
    case "${o}" in
        d) DEB=$(readlink -f $OPTARG);;
        s) BUILD_PATH=$OPTARG;;
        o) OUTFILE=$OPTARG;;
        *) usage ;;
    esac
done

if [ -z "$DEB" ]; then
  DEB=$BUILD_PATH/$(cd $BUILD_PATH; ls -1 insightcae*.deb|sort -g|head -n 1)
fi

VERSION=$(basename $DEB|sed -e 's/^.*_\(.*\)-.*~.*_.*.deb/\1/g')
PACKAGE=$(basename $DEB|sed -e 's/^\([^_]*\)_.*$/\1/g')


WSLNAME=insightcae-wsl-$OS-$VER
if [ $BRANCH == "master" ]; then
 REPOURL1="http://downloads.silentdynamics.de/ubuntu_dev"
 REPOURL="http://downloads.silentdynamics.de/ubuntu"
elif [ $BRANCH == "next-release" ]; then
 REPOURL="http://downloads.silentdynamics.de/ubuntu_dev"
else
 REPOURL1="http://downloads.silentdynamics.de/ubuntu_dev"
 REPOURL="https://$REPO_CUSTOMER:$REPO_PASSWORD@rostock.kroegeronline.net/customers/$REPO_CUSTOMER"
 WSLNAME=$WSLNAME-$REPO_CUSTOMER
fi

if [ -z "$OUTFILE" ]; then
 OUTFILE=$(pwd)/$WSLNAME-$VERSION.tar.xz
fi
    
UNAME=user
WORKDIR=/home/$UNAME
    
WD=$(mktemp -d)
(
  cd $WD
  
  if [[ ! $DEB -ef $(pwd)/$(basename $DEB) ]]; then
   cp -v $DEB .
  fi
  DEB=$(basename $DEB)

  echo Running in $(pwd)...

  # prepare inputs 
  
  cat > wsl.conf << EOF
[user]
default=$UNAME
EOF

  cat > insight_update.sh << EOF
#!/bin/bash
apt update -y
apt install -y $PACKAGE
echo "=== Update finished ==="
EOF
  chmod +x insight_update.sh

  cat > insight_version.sh << EOF
#!/bin/bash
LC_ALL=C sudo apt policy $PACKAGE
EOF
  chmod +x insight_version.sh

  if [ -n "$REPOURL1" ]; then
   REPO1CMD="RUN add-apt-repository $REPOURL1"
  fi
  
  cat > Dockerfile << EOF
FROM $OS:$VER

RUN apt-get update
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -y ca-certificates sudo locales gnupg software-properties-common apt-utils tzdata
RUN ln -fs /usr/share/zoneinfo/Europe/Berlin /etc/localtime; dpkg-reconfigure --frontend noninteractive tzdata
RUN apt-key adv --fetch-keys http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg
$REPO1CMD
RUN add-apt-repository $REPOURL

COPY $DEB /root
RUN apt-get update; dpkg -i /root/$DEB; apt-get -f install -y; rm /root/$DEB

COPY insight_update.sh /usr/bin
COPY insight_version.sh /usr/bin

RUN useradd -m $UNAME
RUN echo "$UNAME      ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
RUN sed -i "1i source /opt/insightcae/bin/insight_setenv.sh" /home/$UNAME/.bashrc

RUN bash -c "if [ ! -d $WORKDIR ]; then mkdir $WORKDIR; chown user $WORKDIR; fi"

RUN chsh -s /bin/bash $UNAME

COPY wsl.conf /etc
EOF

  echo "Creating $WSLNAME..."
  docker build -t $WSLNAME .
  docker run --name $WSLNAME $WSLNAME /bin/bash
  rm -f *.tar *.tar.gz
  docker export --output $WSLNAME-$VERSION.tar $WSLNAME
  docker rm $WSLNAME
  xz -T0 -9 $WSLNAME-$VERSION.tar  # very slow, but need strong compression to keep file size within EXE file size limit
)

mv $WD/$WSLNAME-$VERSION.tar.xz $OUTFILE
rm -rf $WD
