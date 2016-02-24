#! /bin/bash

function pci {
    PCILIB_PATH=".."
    LD_LIBRARY_PATH="$PCILIB_PATH/pcilib" $PCILIB_PATH/pcitool/pci $*
}

i=1
while [ 1 ]; do
    pci --start-dma dma1r
    for name in /sys/class/fpga/fpga0/kbuf*; do
	bus_addr=0x`cat $name | grep "bus addr" | cut -d ':' -f 2 | sed -e 's/\s\+//g'`
	if [ $((bus_addr % 4096)) -ne 0 ]; then
	    
	    echo "Failed at iteration $i, $name"
	    echo "----------------------"
	    cat $name
	    exit
	fi

    done
    pci  --stop-dma dma1r
    i=$((i + 1))
done
