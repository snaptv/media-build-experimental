#!/bin/bash -eux

HTTP_ROOT=/var/www/html
CURL_NAME=dkms.sh
touch $HTTP_ROOT/$CURL_NAME

# Script that produces the debian package and moves it to be curled from test-servers later

CHANGES=$(git status --porcelain)

if [ "s$CHANGES" != "s" ]; then
    echo Error: Repo must be clean
    echo $CHANGES
    read -n 1 -p "Press enter to continue with dirty repo, ^C otherwise ... " dirty
fi


touch $0

./dkms-build.sh a
set +x && DEB=$(find ~/*.deb -newer $0) && set -x
DEBNAME=$(basename $DEB)
mv $DEB $HTTP_ROOT/dkms
echo "wget http://\$1/dkms/$DEBNAME" >$HTTP_ROOT/$CURL_NAME
echo "sudo dpkg -i $DEBNAME" >>$HTTP_ROOT/$CURL_NAME
# meta: enable syntax "$CURL_NAME <host>" instead of "curl http://<host>/$CURL_NAME | sh -s <host>"
echo "echo \"curl http://\\\$1/$CURL_NAME | sh -s \\\$1\" >$CURL_NAME" >>$HTTP_ROOT/$CURL_NAME
echo "chmod 755 $CURL_NAME" >>$HTTP_ROOT/$CURL_NAME
