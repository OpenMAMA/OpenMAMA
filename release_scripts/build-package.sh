#!/bin/bash

# Make all unhandled erroneous return codes fatal
set -e

# Globals
PREFIX=${PREFIX:-/apps/install}

# Constants
FEDORA=Fedora
RHEL=CentOS
UBUNTU=Ubuntu

test -z "$VERSION" && echo "VERSION must be specified!" && exit $LINENO

if [ -z "$IS_DOCKER_BUILD" ]
then
    echo
    echo "------------------------------- DRAGON ALERT --------------------------------"
    echo "IS_DOCKER_BUILD environment not set. This should be set before launching this"
    echo "script as the root user IN AN ISOLATED DOCKER ENVIRONMENT. It is highly "
    echo "recommended not to run this script in any other environment. Obviously we"
    echo "cannot stop you... but if you try it, you're on your own..."
    echo "-----------------------------------------------------------------------------"
    echo
    exit $LINENO
fi

if [ -f /etc/redhat-release ]
then
    DISTRIB_RELEASE=$(cat /etc/redhat-release | tr " " "\n" | egrep "^[0-9]")
    if [ -f /etc/fedora-release ]
    then
        DISTRIB_ID=$FEDORA
    else
        DISTRIB_ID=$RHEL
    fi
    PACKAGE_TYPE=rpm
    DEPENDS_FLAGS="-d libuuid -d qpid-proton-c -d libevent -d ncurses -d apr -d java-1.8.0-openjdk -d libnsl2"
fi

if [ -f /etc/lsb-release ]
then
    # Will set DISTRIB_ID and DISTRIB_RELEASE
    source /etc/lsb-release
    PACKAGE_TYPE=deb
    DEPENDS_FLAGS="-d uuid -d libevent -d libzmq3 -d openjdk-8-jdk -d ncurses -d libapr1"
fi

if [ "$DISTRIB_ID" == "$FEDORA" ]
then
    DISTRIB_PACKAGE_QUALIFIER=fc${DISTRIB_RELEASE%%.*}
elif [ "$DISTRIB_ID" == "$RHEL" ]
then
    DISTRIB_PACKAGE_QUALIFIER=el${DISTRIB_RELEASE%%.*}
elif [ "$DISTRIB_ID" == "$UBUNTU" ]
then
    DISTRIB_PACKAGE_QUALIFIER=ubuntu${DISTRIB_RELEASE%%.*}
else
    echo "Unsupported distro [$DISTRIB_ID] found: $(cat /etc/*-release)" && exit $LINENO
fi

fpm -s dir \
        -t $PACKAGE_TYPE \
        -m "openmama-users@lists.openmama.org" \
        -C $PREFIX \
        --name openmama \
        --version $VERSION \
        --iteration 1 \
        $DEPENDS_FLAGS \
        -p openmama-$VERSION-1.$DISTRIB_PACKAGE_QUALIFIER.x86_64.$PACKAGE_TYPE \
        --description "OpenMAMA high performance Market Data API" .
