#!/bin/bash -eux

# Script that produces the debian package of the drivers (dkms-binary-style)
# Arguments:
#   No option: assume virtual env, do everything
#   a: "skip install"
#   i: "install only, assume development environment"
#   s: "skip install and stop processing after all files are prepared for compilation"
#   r: "skip install and rebuild"

NAME=snaptv-dddvb-analog
VERSION=0.9.18

KERNEL_VERSION=3.13.0-61-lowlatency
KERNEL_ARCH=x86_64

rebuild=0
src_only=0
install=0
if [ $# -eq 0 ]; then
    install=1
else
    [ "$1" == "i" ] && install=2
    [ "$1" == "s" ] && src_only=1
    [ "$1" == "r" ] && rebuild=1
fi

if [ $install -ge 1 ]; then
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
            wget \
            curl
    if [ $install -eq 1 ]; then
        curl http://apt.snap.tv/bootstrap.sh | sh -s master
        apt-get update
    fi
    apt-get install -y \
            snaptv-package-builder
    [ $install -eq 2 ] && exit
fi

HASH=$(git describe --dirty --always)
LONGVER=$(snap-make-changelog -c | head -1)


KERNEL_VERSION_ARCH=$KERNEL_VERSION/$KERNEL_ARCH
FULL_VERSION=$VERSION-snaptv-$LONGVER
ID=$NAME/$FULL_VERSION
LIB_DIR=/var/lib/dkms/$ID

if [ $rebuild -eq 0 ]; then

make download untar

for file in $(find patches -type f | sort) ; do
    patch -p1 <$file
done

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
[ $src_only -eq 1 ] && exit

else
    rm -fr /usr/src/$NAME-$FULL_VERSION
fi

echo rsync all files including patching to /usr/src/$NAME-$FULL_VERSION
rsync -uav --exclude=.git --exclude=.hg ./ /usr/src/$NAME-$FULL_VERSION >/dev/null

# copy template
sudo rsync -uav /etc/dkms/template-dkms-mkdeb/ /usr/src/$NAME-$FULL_VERSION/$NAME-dkms-mkdeb/

# manipulate postinst, dkms install to the correct kernel
sed s/\\tdkms_configure/"\
\\tdkms ldtarball \/usr\/share\/$NAME-dkms\/$NAME-$FULL_VERSION.dkms.tar.gz\\n\
\\tdkms install -m $NAME -v $FULL_VERSION -k $KERNEL_VERSION\
"/ </usr/src/$NAME-$FULL_VERSION/$NAME-dkms-mkdeb/debian/postinst >postinst
# manipulate control, set dependent of kernel
sed s/Depends:/"Depends: linux-headers-$KERNEL_VERSION, linux-image-$KERNEL_VERSION,"/ </usr/src/$NAME-$FULL_VERSION/$NAME-dkms-mkdeb/debian/control >control
chmod 755 postinst
mv control postinst /usr/src/$NAME-$FULL_VERSION/$NAME-dkms-mkdeb/debian/

dkms build $ID -k $KERNEL_VERSION_ARCH
dkms mkdeb $ID -k $KERNEL_VERSION_ARCH --binaries-only

DEB=$(find $LIB_DIR/deb/ -type f)
echo $DEB

# put the dkms.conf file into the debian packet

mkdir -p ~/"$HASH"/x/DEBIAN
pushd ~/"$HASH"
dpkg -x $DEB x
dpkg -e $DEB x/DEBIAN
mkdir -p x/usr/src/"$NAME"-"$FULL_VERSION"
cp /usr/src/"$NAME"-"$FULL_VERSION"/dkms.conf x/usr/src/"$NAME"-"$FULL_VERSION"
dpkg -b x ~

popd
dkms remove $ID -k $KERNEL_VERSION
rm -fr /usr/src/"$NAME"-"$FULL_VERSION"
rm -fr ~/"$HASH"
