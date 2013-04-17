#! /bin/bash

function pci {
    PCILIB_PATH="/root/pcitool"
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

function enable_hw_trigger {
    usleep 100000
    pci -w control 0xa01
}

rm -f images.raw

enable_hw_trigger &
pid=$!

echo "Starting the grabber"
pci -g -o images.raw --run-time 60000000 --verbose 10 -o /dev/null
wait $pid
