#!/bin/bash

ROOT_PATH=$(dirname $0)

# modules name
MODS='kmod_ipi kmod_uintr'

pushd $ROOT_PATH

make clean
make
for m in ${MODS[@]}; do
  sudo rmmod $m
  sudo insmod $m.ko
  sudo chmod 666 /dev/$m
done

popd
