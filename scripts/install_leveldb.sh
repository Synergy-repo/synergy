#!/bin/bash
#set -xe

ROOT_PATH=$(dirname $0)"/../deps/leveldb"

if [ "$1" = "clean" ]; then
  sudo rm -rf $ROOT_PATH/
  exit 0
fi

git submodule update --init --recursive $ROOT_PATH

pushd $ROOT_PATH

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .

popd
