#!/bin/bash

###################### by Michele Caselle and Uros Stafanovic ##################################################
############ Resent procedure and camera initialization for 10 -bit mode ######################################

error=0
echo " Reset Readout and CMOSIS "
pci -w 0x9040 204 
sleep .1
#echo " Release Reset for Readout"
#pci -w 0x9040 1e0
sleep .1
##################### PLL SET #####################################
val=f501
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10 
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1		
    # exit
fi
sleep 0.01
echo " Start CMOSIS Configuration .."
pci -w 0x9000 f301 
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "bf301" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
val=d207
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01

# Michele 10 - 12 bit mode #
# ###################################################################################################
echo " 10 - bit mode, set Bit_mode "
val=ef00 ######################################################
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
# Michele set ADC_resolution @ 12 bits
echo " 10 bit mode, set ADC resolution 10 bits "
val=f000 # qui for 10 - 11 - 12 bits ########################################################
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01

# adc recommended 28=44
val=e72c
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val "
    error=1
    # exit
fi
sleep 0.01
# ####################################################################################################

################# CAMERA CONFIGURATION ############################################
val=e603
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01

val=d404
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
val=d501
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
# recommended is d840
val=d840
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
# sleep 0.01
# recommended is db40
val=db40
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
val=de65
# val=de0
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
sleep 0.01
val=df6a
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
echo " End CMOSIS Configuration .."
########################################################################################################
echo " Write exp time......"

######################################### EXP TIME #######################################################
val=aa25
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
# val=ab2c
val=ab00
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
# val=acaa
val=ac00
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
####################################################################################################################

sleep 0.01
#pci -w 0x9040 201
sleep 0.01
########################## WRITE THE READOUT NUMBER OF LINE #######################################################
pci -w cmosis_number_lines 1088
#pci -w number_lines 8
sleep 0.01
#################################################################################################################
pci --start-dma dma1
sleep 0.01
#VRAMP 6c is 108
val=e26c
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
#VRAMP 6c is 108
val=e36c
pci -w 0x9000 $val
sleep 0.01
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
sleep 0.01
###################################### NUMBER OF OUTPUT ############################################################
##pci -w 0x9000 0xc803
sleep 0.01
val=c800 
pci -w 0x9000 $val
#pci -r 0x9000 -s 10
sleep 0.01
value=`pci -r 0x9000 -s 8 | grep 9010 | awk '{print $2}' | cut -c 4-8`
if [ "$value" != "b$val" ]; then
    echo "--------------------------------->>>> ERROR! read value: ${value:1:4}, written value: $val"
    error=1
    # exit
fi
#pci -r 0x9000 -s 10

sleep 0.01
#pci -w 0x9000 0xd011
sleep 0.01
#pci -r 0x9000 -s 10

sleep 0.01
#pci -w 0x9000 0xd111
#########################################################################################################
sleep 0.01

#echo " Reset Readout and CMOSIS "
pci -w 0x9040 0x204
sleep .1
echo " Release Reset for Readout"
pci -w 0x9040 0x201
sleep .1

status=`pci -r 0x9050 -s 4 | awk '{print $2$3$4}'`
if [ "$status" != "8449ffff0f0010013ffff111" ]; then
    echo "--------------------------------->>>> ERROR! in the camera status ... "
    error=1
    # exit
fi

#echo "--> $status"

if [ "$error" = "1" ]; then
	echo " Error in the reset and initialization"
else
	echo " Camera READY ........................... OK"		
fi
echo 


