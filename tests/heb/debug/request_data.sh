#!/bin/bash

pci -r dma1 --multipacket -o /dev/null

#echo "Data Reset ... "
pci -w 0x9040 000003f1 
sleep 0.1
pci -w 0x9040 000003f0 

sleep 0.1
#echo "Pilot bunch emulator ..... "
#pci -w 0x9040 400003f0 
sleep 0.2
pci -w 0x9040 03f0 
#pci -r 0x9000 -s 40
#sleep 0.2
#echo "Start data pci. ..... "
pci -w 0x9040 00bf0 

sleep 2

#echo "Stop data acquis...... "
pci -w 0x9040 003f0 
pci -r 0x9000 -s 40
sleep 0.1
#echo "Enable data transfer.... "
pci -w 0x9040 007f0 
