#!/bin/bash

function pci {
    PCILIB_PATH="../.."
    LD_LIBRARY_PATH="$PCILIB_PATH/pcilib" $PCILIB_PATH/pcitool/pci $*
}

size=16
bytes=`expr $size "*" 4`

pci -w xrawdata_packet_length $bytes
pci -w xrawdata_enable_loopback 0
pci -w xrawdata_enable_generator 0

pci --start-dma dma1

while [ $? -eq 0 ]; do
    pci -r dma1 -s 65536 &> /dev/null
done

pci -w xrawdata_enable_loopback 1

for i in `seq 1 10`; do
    pci -w dma1 -s $size "*$i"
    pci -r dma1 -s $size -o bench.out
done

pci --stop-dma dma1

pci -w xrawdata_enable_loopback 0
