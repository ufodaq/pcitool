#!/bin/bash

size=65536

function pci {
    PCILIB_PATH=`pwd`/../../..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci -m kapture $*
}


rm -f bench.out

pci --stop-dma dma0r
pci --start-dma dma0r

# Clean DMA buffers
#while [ $? -eq 0 ]; do
#    pci -r dma0 -s 65536 &> /dev/null
#done

for i in `seq 1 100`; do
    pci -r dma0 --multipacket -s $size -o bench.out
    if [ $? -ne 0 ]; then
	pci --stop-dma dma0r
	exit
    fi
done

pci --stop-dma dma0r

../../../apps/check_counter bench.out
