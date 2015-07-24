#!/bin/bash

# Script that produces the debian package of the drivers (dkms-binary-style)

HASH=$(git describe --dirty --always)

NAME=snaptv-media-build-experimental
VERSION=0.9.18

KERNEL_VERSION=4.0.6-040006-generic
KERNEL_ARCH=x86_64

KERNEL_VERSION_ARCH=$KERNEL_VERSION/$KERNEL_ARCH
FULL_VERSION=$VERSION~$HASH
ID=$NAME/$FULL_VERSION
LIB_DIR=/var/lib/dkms/$ID

CHANGES=$(git status --porcelain | egrep '^ M ')

if [ "$CHANGES" != "" ]; then
    echo Error: Repo must be clean
    exit
fi


sed -f ci-disconnect-patch-relocate <patches/ci-disconnect-patch >patches/ci-disconnect-patch-relocated
make download untar
sudo rsync -uav --exclude=.git --exclude=.hg ./ /usr/src/$NAME-$FULL_VERSION

echo "
PACKAGE_NAME=$NAME
PACKAGE_VERSION=$FULL_VERSION
AUTOINSTALL=y
MAKE[0]='make -j4 all'
BUILD_EXCLUSIVE_KERNEL='^$KERNEL_VERSION'
PATCH[0]=ci-disconnect-patch-relocated" > dkms.conf

num=0
while read module
do
    echo BUILT_MODULE_NAME["$num"]="$module" >> dkms.conf
    echo BUILT_MODULE_LOCATION["$num"]=./v4l >> dkms.conf
    echo DEST_MODULE_LOCATION["$num"]=/updates/dkms >> dkms.conf
    num=$((num+1))
done < modules

sudo mv dkms.conf /usr/src/$NAME-$FULL_VERSION
git clean -fd


sudo dkms build   $ID -k $KERNEL_VERSION_ARCH
sudo dkms install $ID -k $KERNEL_VERSION_ARCH
ls -l /lib/modules/$KERNEL_VERSION/updates/dkms/ | egrep '\.ko$' | awk '{print $NF}' | sed s/\.ko$// | sort >real-modules
sudo dkms remove $ID -k $KERNEL_VERSION

# Produce config file again with the real installed subset of the driver modules

echo "
PACKAGE_NAME=$NAME
PACKAGE_VERSION=$FULL_VERSION
AUTOINSTALL=y
MAKE[0]='make -j4 all'
BUILD_EXCLUSIVE_KERNEL='^$KERNEL_VERSION'" > dkms.conf

num=0
while read module
do
    echo BUILT_MODULE_NAME["$num"]="$module" >> dkms.conf
    echo BUILT_MODULE_LOCATION["$num"]=./v4l >> dkms.conf
    echo DEST_MODULE_LOCATION["$num"]=/updates/dkms >> dkms.conf
    num=$((num+1))
done < real-modules

sudo mv dkms.conf /usr/src/$NAME-$FULL_VERSION
sudo dkms build $ID -k $KERNEL_VERSION_ARCH

sudo dkms mkdeb $ID -k $KERNEL_VERSION_ARCH --binaries-only
ls -l $LIB_DIR/deb
sudo cp $LIB_DIR/deb/* ~
sudo dkms remove $ID -k $KERNEL_VERSION
