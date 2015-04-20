#!/bin/bash

size=65536

function pci {
    PCILIB_PATH=`pwd`/../../..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci -m kapture $*
}


rm -f bench.out

echo "Stopping DMA and skipping exiting data..."
pci --stop-dma dma0r
echo "Starting DMA..."
pci --start-dma dma0r
echo "Enabling data generation..."
pci -w 0x4 0x1

# Clean DMA buffers
#while [ $? -eq 0 ]; do
#    pci -r dma0 -s 65536 &> /dev/null
#done

echo "Reading the data from DMA..."
for i in `seq 1 100`; do
    pci -r dma0 --multipacket -s $size -o bench.out
    if [ $? -ne 0 ]; then
	echo "Stopping DMA due to the error..."
	pci --stop-dma dma0r
	exit
    fi
done

echo "Stopping DMA..."
pci --stop-dma dma0r

../../../apps/check_counter bench.out

#pci -r 0 -s 32
