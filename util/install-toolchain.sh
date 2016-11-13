#!/bin/bash
URL=https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2
TOOLCHAIN=gcc-arm-none-eabi-5_2-2015q4
if [[ ! -d "$HOME/toolchains/gcc-arm-embedded" ]]; then
    mkdir -p ~/toolchains
    wget -qO- $URL | tar xj -C ~/toolchains/
    ln -s $TOOLCHAIN ~/toolchains/gcc-arm-embedded
fi;
