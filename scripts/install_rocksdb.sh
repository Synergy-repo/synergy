#!/bin/bash
set -xe

#path to rocksdb
ROOT_PATH=$(dirname $0)"/../deps/rocksdb"

if [ "$1" = "clean" ]; then
  sudo rm -rf $ROOT_PATH/
  exit 0
fi

git submodule update --init $ROOT_PATH

pushd $ROOT_PATH

#git checkout v9.4.0
git checkout v5.15.10

CFLAGS="\
-Wno-deprecated-copy \
-Wno-pessimizing-move \
-Wno-redundant-move" \
make -B -j$(nproc) static_lib

popd
