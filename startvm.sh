#!/bin/bash

sudo qemu-system-x86_64 -enable-kvm -M q35 -m 2G -boot d /home/connor/Virtual\ \Machines/VirtualDebug/linux.qcow2 -device vfio-pci,host=07:00.0 -vga std -smp 8 -trace events=/home/connor/Virtual\ \Machines/events.txt -monitor stdio 

This is my qemu startup script. Nothing special.
