#!/bin/bash

echo " Reset Readout and CMOSIS "
pci -w 0xd2009040 1e5
echo " Release Reset for Readout"
pci -w 0xd2009040 1e1

echo " Start CMOSIS Configuration .."
pci -w 0xd2009000 f301 
pci -r 0xd2009000 -s 4

pci -w 0xd2009000 d207
pci -r 0xd2009000 -s 4

pci -w 0xd2009000 8101
pci -r 0xd2009000 -s 4

pci -w 0xd2009000 8200
pci -r 0xd2009000 -s 4
echo " End CMOSIS Configuration .."
pci -w 0xd2009040 3e1

pci -r 0xd2009000 -s 100
