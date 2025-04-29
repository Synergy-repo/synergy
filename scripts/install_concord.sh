#!/bin/bash
set -xe

#path to dpdk
ROOT_PATH=$(dirname $0)"/../deps/concord"

if [ "$1" = "clean" ]; then
  sudo rm -rf $ROOT_PATH
  exit 0
fi

git submodule update --init $ROOT_PATH

# Ubuntu
VERSION=$(grep -oP '(?<=VERSION_ID=")\d+' /etc/os-release)
if [[ $VERSION -gt 18 ]]; then

  wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -

  # focal is last version with clang-9 available
  echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-9 main" \
  | sudo tee /etc/apt/sources.list.d/llvm-9.list

  wget http://es.archive.ubuntu.com/ubuntu/pool/main/libf/libffi/libffi7_3.3-4_amd64.deb
  sudo dpkg -i libffi7_3.3-4_amd64.deb

fi

pushd $ROOT_PATH

git apply ../concord.patch

bash setup.sh

popd
