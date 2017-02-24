#!/bin/bash -eux

# Script that produces the debian package of the drivers (dkms-binary-style)
# Arguments:
#   No option: assume virtual env, do everything except fetching external repos
#   i: "install only, assume development environment"
#   s: "Fetch external repo"
#   p: "Patch sources so they are prepared for compilation"
#   r: "build - rebuild"
#   c: clean

if [ "$EUID" -ne 0 ] ; then echo "Please run as root"; exit; fi

NAME=snaptv-dddvb-analog
VERSION=0.9.18

KERNEL_VERSION=3.13.0-61-lowlatency
KERNEL_ARCH=x86_64

install=0
fetch_src=0
patch_src=0
rebuild=0

if [ $# -eq 0 ]; then
    install=1
    fetch_src=0
    patch_src=1
    rebuild=1
else
    if [ "$1" == "c" ]; then
        git clean -fd
        git reset --hard
        exit
    fi
    [ "$1" == "i" ] && install=2
    [ "$1" == "s" ] && fetch_src=1
    [ "$1" == "p" ] && patch_src=1
    [ "$1" == "r" ] && rebuild=1
fi

echo i $install s $fetch_src p $patch_src r $rebuild

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

if [ $fetch_src -eq 1 ]; then

# files are fetched from external repos and stored in /linux and /experimental/v4l-dvb-saa716x

rm -r linux
rm -r experimental/v4l-dvb-saa716x
cp -r linux-init linux
make download untar
rm -r $(find -name .hg)
rm linux/linux-media.tar.bz2*

fi

if [ $patch_src -eq 1 ]; then

for file in $(find patches -type f | sort) ; do
    patch -p1 <$file
done

fi

if [ $rebuild -eq 1 ]; then

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

echo rsync all files including patching to /usr/src/$NAME-$FULL_VERSION
rsync -uav --exclude=.git --delete-excluded ./ /usr/src/$NAME-$FULL_VERSION >/dev/null

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

fi