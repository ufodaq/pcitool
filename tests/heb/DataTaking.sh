#!/bin/bash

#Channel 1 --> 05
#Channel 2 --> 0B
#Channel 1&2 --> 0F
#Channel 3 --> 13
#Channel 4 --> 23
#Channel 3&4 --> 33
#ALL --> 3F

#rm *.out
pci -r dma1 --multipacket -o /dev/null

echo "Start DMA ..... "
#pci --start-dma dma1
sleep 0.2

echo "Data Reset ... "
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
echo "Start data pci. ..... "
pci -w 0x9040 00bf0 

sleep 2

echo "Stop data acquis...... "
pci -w 0x9040 003f0 
pci -r 0x9000 -s 40
sleep 0.1
echo "Enable data transfer.... "
pci -w 0x9040 007f0 
exit
sleep 2
pci -r dma1 -o run_num_$1.out --multipacket
sleep 2

pci -w 0x9040 003f0 
#echo "Status ... "



pci -r 0x9000 -s 40

   status=`pci -r 0x9050 -s 1 | awk '{print $2$3$4}'`
    if [ "$status" != "85000021" ]; then
       echo "--------------------------------->>>> ERROR! ... "
       error=1
       exit
    else 
       echo " Status 1 -> OK "	
    fi	
	
   status=`pci -r 0x9000 -s 1 | awk '{print $2$3$4}'`
    if [ "$status" != "01000021" ]; then
       echo "--------------------------------->>>> ERROR! ... "
       error=1
        exit
    else 
       echo " Status 1 Readout -> OK "	
   fi
    
status=`pci -r 0x9008 -s 1 | awk '{print $2$3$4}'`
    if [ "$status" != "01000021" ]; then
       echo "--------------------------------->>>> ERROR! ... "
       error=1
        exit
    else 
       echo " Status 2 Readout -> OK "	
   fi

status=`pci -r 0x9010 -s 1 | awk '{print $2$3$4}'`
    if [ "$status" != "01000021" ]; then
       echo "--------------------------------->>>> ERROR! ... "
       error=1
        exit
    else 
       echo " Status 3 Readout -> OK "	
   fi

status=`pci -r 0x9018 -s 1 | awk '{print $2$3$4}'`
    if [ "$status" != "01000021" ]; then
       echo "--------------------------------->>>> ERROR! ... "
       error=1
        exit
    else 
       echo " Status 4 Readout -> OK "	
   fi
