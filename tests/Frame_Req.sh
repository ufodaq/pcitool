#!/bin/bash

echo " Reset Readout and CMOSIS "
pci -w 0xd2009040 1e5
pci -r 0xd2009040 -s 10
echo " Release Reset for CMOSIS"
pci -w 0xd2009040 1e1
pci -r 0xd2009040 -s 10

sleep 1
echo " Start CMOSIS Configuration .."
pci -w 0xd2009000 f301 
pci -r 0xd2009000 -s 10

usleep 1000
pci -w 0xd2009000 d207
pci -r 0xd2009000 -s 10
usleep 1000
echo " Number of rows set here "
pci -w 0xd2009000 8110
pci -r 0xd2009000 -s 10

usleep 1000
pci -w 0xd2009000 8200
pci -r 0xd2009000 -s 10

sleep 1
echo "Set packet siye 1024 .. "
pci -w xrawdata_packet_length 4096
pci --start-dma dma1
sleep 1

echo "Send frame request ... "
pci -w 0xd2009040 1e9
pci -w 0xd2009040 1e1

echo "Status ... "
pci -r 0xd2009000 -s 100

echo "Enable Readout ... "
pci -w 0xd2009040 3e1
pci -r 0xd2009000 -s 100

for i in `seq 1 10`; do
#  pci -r dma1 -s 1024 -o bench.out
  pci -r dma1 -s 4096
done

pci --stop-dma dma1

