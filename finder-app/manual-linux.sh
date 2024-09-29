#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.4.284
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
ROOT_FS_DIRECTORIES=("bin" "dev" "etc" "home" "lib" "lib64" "proc" "sbin" "sys" "tmp" "usr" "var" "usr/bin" "usr/lib" "usr/sbin" "var/log")
TOOLCHAIN_LIB="$(realpath $(dirname $(which aarch64-none-linux-gnu-gcc))/..)/aarch64-none-linux-gnu/libc"

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
elif [ ! -d $1 ]; then
	echo "Outdir must be a directory. Using default directory ${OUTDIR} for output."
else
    OUTDIR=$(realpath $1)
	echo "Using passed directory ${OUTDIR} for output"
fi

ROOTFS="${OUTDIR}/rootfs"

# ========================================
#              ***KERNEL***
# ========================================

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # build steps
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper      # clean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig     # .config
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all           # kernel image
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs          # device tree
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image $OUTDIR


# ========================================
#          ***ROOT FILESYSTEM***
# ========================================

cd "$OUTDIR"
if [ -d "${ROOTFS}" ]
then
	echo "Deleting rootfs directory at ${ROOTFS} and starting over"
    sudo rm  -rf "$ROOTFS"
fi

echo "Creating the staging directory for the root filesystem"
mkdir -p "$ROOTFS" && cd "$ROOTFS"

for root_dir in "${ROOT_FS_DIRECTORIES[@]}"
do
    mkdir -p "${root_dir}"
done
echo "Created the root filesystem at ${ROOTFS}"


# ========================================
#              ***BUSYBOX***
# ========================================

echo "setting up busybox"
cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
fi

cd busybox

if [ "$(git describe --tags --abbrev=0)" != "${BUSYBOX_VERSION}" ]; then
    git checkout ${BUSYBOX_VERSION}
    make distclean
    make defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
elif [ ! -e "${OUTDIR}/busybox/busybox" ]; then
    make distclean
    make defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
fi
make CONFIG_PREFIX=${ROOTFS} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd "$ROOTFS"
echo "Resolving Library Dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
echo "copying ${TOOLCHAIN_LIB}/lib/ld-linux-aarch64.so.1 to ${ROOTFS}/lib"
cp "${TOOLCHAIN_LIB}/lib/ld-linux-aarch64.so.1" "${ROOTFS}/lib"
echo "copying ${TOOLCHAIN_LIB}/lib64/libm.so.6 to ${ROOTFS}/lib64"
cp "${TOOLCHAIN_LIB}/lib64/libm.so.6"           "${ROOTFS}/lib64"
echo "copying ${TOOLCHAIN_LIB}/lib64/libresolv.so.2 to ${ROOTFS}/lib64"
cp "${TOOLCHAIN_LIB}/lib64/libresolv.so.2"      "${ROOTFS}/lib64"
echo "copying ${TOOLCHAIN_LIB}/lib64/libc.so.6 to ${ROOTFS}/lib64"
cp "${TOOLCHAIN_LIB}/lib64/libc.so.6"           "${ROOTFS}/lib64"


# ========================================
#            ***DEVICE NODES***
# ========================================

echo "making device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1


# ========================================
#           ***WRITER UTILITY***
# ========================================

echo "compiling and copying the finder app and assignment files"
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

cp makefile writer finder.sh ../conf/username.txt ../conf/assignment.txt finder-test.sh ${ROOTFS}/home
cp autorun-qemu.sh ${ROOTFS}/home


# ========================================
#             ***INITRAMFS***
# ========================================

echo "creating initramfs"
# chown the root directory
cd $ROOTFS
sudo chown -R root:root *

# create .cpio.gz
cd $ROOTFS
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio