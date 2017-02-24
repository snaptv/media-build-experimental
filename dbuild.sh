#!/bin/bash -eux

if [ "$EUID" -ne 0 ] ; then echo "Please run as root"; exit; fi

HTTP_ROOT=/var/www/html
CURL_NAME=dkms.sh
touch $HTTP_ROOT/$CURL_NAME

# Script that produces the debian package and moves it to be curled from test-servers later

dkms_opt_p=0

if [ $# -gt 0 ]; then
    if [ "$1" == "clean" ]; then
        git clean -fd
        git reset --hard
        exit
    fi
    [ "$1" == "patch" ] && dkms_opt_p=1
fi

[ $dkms_opt_p -eq 1 ] && ./dkms-build.sh p
./dkms-build.sh r

set +x && DEB=$(find ~/*.deb -newer $0) && set -x
DEBNAME=$(basename $DEB)
mv $DEB $HTTP_ROOT/dkms
echo "wget http://\$1/dkms/$DEBNAME" >$HTTP_ROOT/$CURL_NAME
echo "sudo dpkg -i $DEBNAME" >>$HTTP_ROOT/$CURL_NAME
# meta: enable syntax "$CURL_NAME <host>" instead of "curl http://<host>/$CURL_NAME | sh -s <host>"
echo "echo \"curl http://\\\$1/$CURL_NAME | sh -s \\\$1\" >$CURL_NAME" >>$HTTP_ROOT/$CURL_NAME
echo "chmod 755 $CURL_NAME" >>$HTTP_ROOT/$CURL_NAME
