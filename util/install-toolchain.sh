#!/bin/bash
set -eo pipefail
URL=https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2
TOOLCHAIN=gcc-arm-none-eabi-5_2-2015q4
TOOLCHAINS=$HOME/toolchains
if [[ ! -d "${TOOLCHAINS}/gcc-arm-embedded" ]]; then
    echo "Installing $TOOLCHAIN from $URL to ${TOOLCHAINS}"
    mkdir -p ${TOOLCHAINS}
    wget -qO- $URL | tar xj -C ${TOOLCHAINS}
    ln -s $TOOLCHAIN ${TOOLCHAINS}/gcc-arm-embedded
fi;

EXISTING_TOOLCHAIN=`readlink -f "${TOOLCHAINS}/gcc-arm-embedded"`
echo "Current toolchain is $EXISTING_TOOLCHAIN"

TOOLCHAIN_VER=`${TOOLCHAINS}/gcc-arm-embedded/bin/arm-none-eabi-gcc --version | head -n 1`
echo "Installed toolchain version is $TOOLCHAIN_VER"
