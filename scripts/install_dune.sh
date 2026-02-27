#!/bin/bash

#path to dune
ROOT_PATH=$(dirname $0)"/../deps/dune"

if [ "$1" = "clean" ]; then
  sudo rm -rf $ROOT_PATH/
  exit 0
fi

git submodule update --init $ROOT_PATH

pushd $ROOT_PATH

make
sudo rmmod dune
sudo insmod ./kern/dune.ko

popd
