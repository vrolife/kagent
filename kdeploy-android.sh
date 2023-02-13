#!/bin/sh
set -e

MODULE=hello
if test -n "$1"; then
    MODULE="$1"
fi

llvm-strip -x out/build/dev-arm64/kdeploy
adb push out/build/dev-arm64/kdeploy /data/local/tmp
adb push out/build/dev-arm64/modules/$MODULE/$MODULE.ko /data/local/tmp
adb shell su -c "/data/local/tmp/kdeploy -m /data/local/tmp/$MODULE.ko -b /dev/block/by-name/boot -o /data/local/tmp/out.ko"
adb pull /data/local/tmp/out.ko
