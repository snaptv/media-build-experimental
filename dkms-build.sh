#!/bin/bash -eux

# Script that produces the debian package of the drivers (dkms-binary-style)

NAME=snaptv-dddvb-analog
VERSION=0.9.18

KERNEL_VERSION=3.13.0-61-lowlatency
KERNEL_ARCH=x86_64

apt-get update
apt-get install -y \
        bzip2 \
        debhelper \
        dkms \
        dpkg-dev \
        git \
        libproc-processtable-perl \
        mercurial \
        linux-headers-$KERNEL_VERSION \
        sudo \
        wget

HASH=$(git describe --dirty --always)


KERNEL_VERSION_ARCH=$KERNEL_VERSION/$KERNEL_ARCH
FULL_VERSION=$VERSION~$HASH
ID=$NAME/$FULL_VERSION
LIB_DIR=/var/lib/dkms/$ID

CHANGES=$(git status --porcelain)

if [ "s$CHANGES" != "s" ]; then
    echo Error: Repo must be clean
    echo $CHANGES
    exit
fi


make download untar

for file in $(find patches -type f | sort) ; do
    patch -p1 <$file
done

echo rsync all files including patching to /usr/src/$NAME-$FULL_VERSION
sudo rsync -uav --exclude=.git --exclude=.hg ./ /usr/src/$NAME-$FULL_VERSION >/dev/null

modules=$(cat modules)

echo "
PACKAGE_NAME=$NAME
PACKAGE_VERSION=$FULL_VERSION
AUTOINSTALL=y
MAKE[0]='make -j4 all'
BUILD_EXCLUSIVE_KERNEL='^$KERNEL_VERSION'" > dkms.conf
num=0
for module in $modules; do
    echo BUILT_MODULE_NAME["$num"]="$module" >> dkms.conf
    echo BUILT_MODULE_LOCATION["$num"]=./v4l >> dkms.conf
    echo DEST_MODULE_LOCATION["$num"]=/updates/dkms >> dkms.conf
    num=$((num+1))
done

sudo mv dkms.conf /usr/src/$NAME-$FULL_VERSION
git clean -fd


sudo dkms build $ID -k $KERNEL_VERSION_ARCH
sudo dkms mkdeb $ID -k $KERNEL_VERSION_ARCH --binaries-only

DEB=$(find $LIB_DIR/deb/ -type f)
echo $DEB

# put the dkms.conf file into the debian packet

mkdir -p ~/"$HASH"/x/DEBIAN
pushd ~/"$HASH"
sudo dpkg -x $DEB x
sudo dpkg -e $DEB x/DEBIAN
sudo mkdir -p x/usr/src/"$NAME"-"$FULL_VERSION"
sudo cp /usr/src/"$NAME"-"$FULL_VERSION"/dkms.conf x/usr/src/"$NAME"-"$FULL_VERSION"
sudo dpkg -b x ~

popd
sudo dkms remove $ID -k $KERNEL_VERSION
sudo rm -fr /usr/src/"$NAME"-"$FULL_VERSION"
sudo rm -fr ~/"$HASH"
