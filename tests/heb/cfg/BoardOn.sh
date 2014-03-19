#!/bin/bash

echo " ************************************************************** "
echo "				Start DMA"
echo " ************************************************************** "

pci --start-dma dma1
sleep 0.5
pci  --list-dma-engines


echo " ************************************************************** "
echo " 			Board ON procedure"
echo " ************************************************************** "

pci -w 0x9040 0x01
sleep 1

echo "switch ON the power supply  --> FIRST <--"
echo "Press a key to continue ...."
read -n 1 -s

echo "Switch ON T/Hs"
pci -w 0x9040 0x3C1
pci -r 0x9040 -s1

echo "switch ON the power supply  --> SECOND <--"
echo "Press a key to continue ...."
read -n 1 -s

echo "Switch ON ADCs"
pci -w 0x9040 0x3F1
pci -r 0x9040 -s1
sleep 0.1


pci -w 0x9040 0x3F0 
pci -r 0x9040 -s1
sleep 1

echo " Status ................... "
pci -r 0x9000 -s 40



./PLL_conf_calib_3001.sh

echo " Status ................... "
pci -r 0x9000 -s 40


echo " ************************************************************** "
echo "				Board Ready"
echo " ************************************************************** "


echo " --> remember to run: ./Set_Default.sh"
