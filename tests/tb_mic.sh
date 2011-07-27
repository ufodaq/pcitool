
pci --stop-dma dma1
pci -w xrawdata_packet_length 4096
pci --start-dma dma1
pci -w 0xd2009040 1e9
pci -w 0xd2009040 1e1
pci -w 0xd2009040 3e1
pci -r 0xd2009000 -s 100
#pci -r dma1 -s 1024
#LD_LIBRARY_PATH=/root/pcitool ./pci -g -o 1.image
#pci --stop-dma dma1
