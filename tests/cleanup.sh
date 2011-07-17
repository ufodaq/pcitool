#!/bin/bash

function pci {
    PCILIB_PATH="/root/pcitool"
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

pci --start-dma dma1r
pci -w dma1r_reset_request 1
pci -w dma1r_reset 1
pci -r dma1r_running

pci --clean-kernel-memory dma
