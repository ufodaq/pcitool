#! /bin/bash
devdir=`ls -d /sys/bus/pci/devices/*/driver/module/drivers/pci:pciDriver`
if [ $? -ne 0 ]; then
    echo "Xilinx device doesn't exist, rescanning..."
    echo 1 > /sys/bus/pci/rescan
    exit
else
    device=`echo $devdir | head -n 1 | cut -c 27-33`
    echo "Xilinx is located at: " $device
fi
echo "remove devices"
echo  1 > /sys/bus/pci/devices/0000\:${device:0:2}\:${device:3:4}/remove
sleep 1
echo "rescan"
echo 1 > /sys/bus/pci/rescan
sleep 1
echo "remove driver"
rmmod pciDriver 
sleep 1
echo "instantiate driver"
modprobe pciDriver
sleep 1
echo "set bus master dma"
dev=$device  
echo Enabling bus mastering on device $dev
setpci -s $dev 4.w=0x07
