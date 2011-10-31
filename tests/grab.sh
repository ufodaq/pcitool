#! /bin/bash

function pci {
    PCILIB_PATH="/root/pcitool"
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

rm image.raw

echo "Reset..."
pci --reset
echo "Stop DMA..."
pci --stop-dma
echo "Start DMA..."
pci --start-dma dma1
echo "Request..."
pci -g -o image.raw
