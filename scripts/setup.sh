#!/bin/bash
source $(dirname $0)/../MACHINE_CONFIG

# disable whatchdog
sudo sysctl -w kernel.watchdog=0

# set hugepages
sudo bash -c "echo 8192 > /sys/devices/system/node/node${NUMA_ID}/hugepages/hugepages-2048kB/nr_hugepages"

# load kmod_ipi
$(dirname $0)/../kmod/load_mods.sh

# disabe turbo boost
$(dirname $0)/turbo.sh disable > /dev/null

# bind NIC
if [[ "$NIC_BIND" != "false" ]]; then
  sudo modprobe uio
  sudo insmod $(dirname $0)/../deps/dpdk-kmods/linux/igb_uio/igb_uio.ko
  sudo ip link set down dev ${NIC_NAME}
  sudo $(dirname $0)/../deps/dpdk/usertools/dpdk-devbind.py -b igb_uio ${NIC_PCI}
fi

# create database
#sudo mkdir -p /tmpfs
#sudo mount -t tmpfs -o size=50G,mode=1777 tmpfs /tmpfs
#make database -C $(dirname $0)/..
