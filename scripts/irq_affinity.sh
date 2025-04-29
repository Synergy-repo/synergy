#!/bin/bash

# send interruptions only to this cpu(s)
CPUS="0"

for i in $(ls /proc/irq | grep -v default); do
  echo $CPUS > /proc/irq/$i/smp_affinity_list
done
