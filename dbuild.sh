#!/bin/bash -eux

HTTP_ROOT=/var/www/html
CURL_NAME=dkms.sh
touch $HTTP_ROOT/$CURL_NAME

# Script that produces the debian package and moves it to be curled from test-servers later

dkms_opt="a"

if [ $# -gt 0 ]; then
    if [ "$1" == "clean" ]; then
        git clean -fd
        git reset --hard
        exit
    fi
    if [ "$1" == "rebuild" ]; then
        dkms_opt="r"
    fi
fi

CHANGES=$(git status --porcelain)

if [ "s$CHANGES" != "s" ]; then
    if [ $dkms_opt == "a" ]; then
        echo Warning: Repo must be clean - run './dbuild.sh clean' to clean or './dbuild.sh rebuild' to build again
        exit
    fi
else
    if [ $dkms_opt == "r" ]; then
        echo Warning: Cannot rebuild a clean repo
        exit
    fi
fi


./dkms-build.sh $dkms_opt
set +x && DEB=$(find ~/*.deb -newer $0) && set -x
DEBNAME=$(basename $DEB)
mv $DEB $HTTP_ROOT/dkms
echo "wget http://\$1/dkms/$DEBNAME" >$HTTP_ROOT/$CURL_NAME
echo "sudo dpkg -i $DEBNAME" >>$HTTP_ROOT/$CURL_NAME
# meta: enable syntax "$CURL_NAME <host>" instead of "curl http://<host>/$CURL_NAME | sh -s <host>"
echo "echo \"curl http://\\\$1/$CURL_NAME | sh -s \\\$1\" >$CURL_NAME" >>$HTTP_ROOT/$CURL_NAME
echo "chmod 755 $CURL_NAME" >>$HTTP_ROOT/$CURL_NAME
