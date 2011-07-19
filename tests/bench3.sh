#!/bin/bash

function pci {
    PCILIB_PATH="/root/pcitool"
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

size=16
bytes=`expr $size "*" 4`

pci -w xrawdata_packet_length $bytes

pci --start-dma dma1
pci -w control 0x1e5
sleep 1
pci -w control 0x1e1

for i in `seq 1 10`; do
    pci -w control 0x1e1
    pci -w dma1 -s $size "*$i"
    pci -w control 0x3e1
    pci -r dma1 -s $size -o bench.out
done

pci --stop-dma dma1

