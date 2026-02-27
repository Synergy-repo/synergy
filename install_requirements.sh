#!/bin/bash

PKGS='
cmake
python3-pyelftools
python3-pip
pkg-config
rdma-core
libibverbs-dev
libnuma-dev
msr-tools
libgflags-dev
libsnappy-dev
zlib1g-dev
libbz2-dev
liblz4-dev
libzstd-dev
'

# Ubuntu
VERSION=$(grep -oP '(?<=VERSION_ID=")\d+' /etc/os-release)

set -x

sudo apt update
sudo apt install -y ${PKGS}

# Check if the major version is below 20
if [[ $VERSION -lt 20 ]]; then
  sudo pip3 install meson ninja
else
  sudo apt install meson ninja-build
fi

