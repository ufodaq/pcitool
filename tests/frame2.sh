#!/bin/bash

function pci {
    PCILIB_PATH="/root/pcitool"
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

pci --stop-dma dma1
#pci --reset

echo "Set packet size 1024 .. "
pci -w cmosis_number_lines 1088
#pci -w xrawdata_packet_length 4096
pci --start-dma dma1
usleep 1000

echo "Send frame request ... "
pci -w control 1e9
usleep 1000
pci -w control 1e1
usleep 300000
pci -w control 1e9
usleep 1000
pci -w control 1e1
usleep 3000

echo "Enable Readout ... "
pci -w control 3e1

usleep 100000

pci -r dma1 -o bench.out --multipacket --timeout 1000000

pci -w control 1e1

pci --stop-dma dma1

