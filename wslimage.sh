#!/bin/bash

##
## This script runs in the superbuild root directory
##

set -e

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/setup_environment.sh

BUILD_PATH=$SCRIPTPATH/insight-windows-build
SUPERBUILD_BUILD_PATH=$SCRIPTPATH/opt/insight-build

WSLNAME=insightcae-ubuntu-1804
if [ $BRANCH == "master" ]; then
 REPOURL="http://downloads.silentdynamics.de/ubuntu"
 PACKAGE=insightcae-ce
elif [ $BRANCH == "next-release" ]; then
 REPOURL="http://downloads.silentdynamics.de/ubuntu_dev"
 PACKAGE=insightcae-ce
else
 REPOURL="https://$REPO_CUSTOMER:$REPO_PASSWORD@rostock.kroegeronline.net/customers/$REPO_CUSTOMER"
 PACKAGE=insightcae
 WSLNAME=$WSLNAME-$REPO_CUSTOMER
fi
    
UNAME=user
WORKDIR=/home/$UNAME
    
(
  mkdir -p wsl
  cd wsl

  echo Running in $(pwd)...

  # prepare inputs 
  
  cat > wsl.conf << EOF
[user]
default=user
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

  DEB=$(cd $SUPERBUILD_BUILD_PATH; ls -1 insightcae*.deb|sort -g|head -n 1)
  VERSION=$(echo $DEB|sed -e 's/^.*_\(.*\)-.*~.*_.*.deb/\1/g')
  cp -v $SUPERBUILD_BUILD_PATH/$DEB .

  cat > Dockerfile << EOF
FROM scratch
ADD "ubuntu-18.04-server-cloudimg-amd64-wsl.rootfs.tar.gz" /

RUN apt-get update
RUN apt-get install -y ca-certificates sudo
RUN apt-key adv --fetch-keys http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg
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

  docker build -t $WSLNAME .
  docker run --name $WSLNAME $WSLNAME /bin/bash
  rm -f *.tar *.tar.gz
  docker export --output $WSLNAME-$VERSION.tar $WSLNAME
  docker rm $WSLNAME
  gzip $WSLNAME-$VERSION.tar
)

