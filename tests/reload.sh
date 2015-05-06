#! /bin/bash

device=`lspci -n | grep -m 1 "10ee:" | awk '{print $1}'`
if [ -z "$device" ]; then
    echo "Xilinx device doesn't exist, rescanning..."
    echo 1 > /sys/bus/pci/rescan
    exit
else
    echo "Xilinx is located at: " $device
fi
echo "remove driver"
rmmod pciDriver 
echo "remove devices"
echo  1 > /sys/bus/pci/devices/0000\:${device:0:2}\:${device:3:4}/remove
sleep 1
echo "rescan"
echo 1 > /sys/bus/pci/rescan
sleep 1
echo "instantiate driver"
modprobe pciDriver
# for devices with different ID
#echo "10ee 6028" > /sys/bus/pci/drivers/pciDriver/new_id
pci -i
#echo Enabling bus mastering on device $dev
#setpci -s $device 4.w=0x07
