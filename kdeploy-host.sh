#!/bin/sh
set -e

./out/build/dev-host/kagent/kdeploy -m kagent/obj-x86_64-linux-gnu/module.ko -k ./vmlinux.bin -s vmlinux.map
