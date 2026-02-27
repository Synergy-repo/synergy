#!/bin/bash

TARGET_CPU=30
CPU_MASK=$(printf "%x" $((1 << TARGET_CPU)))

sudo systemctl stop irqbalance

echo "Redirecting all IRQs to CPU $TARGET_CPU (mask 0x$CPU_MASK)..."

for IRQ in $(ls /proc/irq | grep -E '^[0-9]+$'); do
    #echo "Redirecting IRQ $IRQ to CPU $TARGET_CPU"
    echo $CPU_MASK | sudo tee /proc/irq/$IRQ/smp_affinity
done

