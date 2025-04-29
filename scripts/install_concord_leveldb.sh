#!/bin/bash

git submodule update --init --recursive $(dirname $0)/../deps/concord_leveldb

pushd $(dirname $0)/../deps/concord_leveldb

#Compile default library (whitout concord pass)
##mkdir -p build
##pushd build
##cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
##popd

# Compile using concord pass
make -f concord.mk all -j3
popd
