#!/bin/bash

function pci {
    PCILIB_PATH="/root/pcitool"
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

rm -f bench.out

echo "Set FFFF the frame space .."
pci -w 0x9180 fff 

echo "Set the number of frames .."
pci -w reg9170 55

pci --start-dma dma1

echo "Send frame request ... "
pci -w control 1f1
usleep 100000
pci -w control 1e1


echo "Enable Readout ... "
pci -w control 3e1
pci -r dma1 -o bench.out --multipacket
pci -w control 1e1

pci --stop-dma dma1

