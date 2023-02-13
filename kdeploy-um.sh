#!/bin/sh
set -e

make ARCH=x86_64 -C kagent clean
make ARCH=x86_64 -C kagent

UM_DIR="$1"

if test -z "$UM_DIR"; then
    echo "Missing um dir"
    exit -1
fi

./out/build/dev-host/kagent/kdeploy -m kagent/obj-x86_64-linux-gnu/module.ko -k $UM_DIR/vmlinux.bin -s $UM_DIR/System.map

cp out.ko $UM_DIR/root

gdb --args $UM_DIR/vmlinux \
    root=/dev/root \
    rootflags=$UM_DIR/root \
    rootfstype=hostfs \
    debug
