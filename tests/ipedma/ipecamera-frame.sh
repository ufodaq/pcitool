#!/bin/bash

TESTS_PATH="`dirname \"$0\"`"
TESTS_PATH="`( cd \"$TESTS_PATH\" && pwd )`"

function pci {
    PCILIB_PATH=$TESTS_PATH/../../..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci -m ipedma $*
}


rm bench.out

pci --stop-dma dma0r
#pci --reset

echo "Set packet size 1024 .. "
#pci -w cmosis_number_lines 1088
#pci -w xrawdata_packet_length 4096
pci --start-dma dma0r
usleep 1000

pci -w 0x90a8 0x0a
pci -w 0x90a0 0x0a

echo "Send frame request ... "
# Single frame
pci -w 0x9040 80000209
# Stimuli
#pci -w 0x9040 800002f1
# Streaming
#pci -w 0x9040 80000a01
usleep 100000
pci -w 0x9040 80000201
usleep 100000

echo "Enable Readout ... "
#pci -w control 3e1
pci -w 0x4 0x1

usleep 100000

pci -r dma0 -o bench.out --multipacket

pci -w 0x9040 80000001

pci --stop-dma dma0r

