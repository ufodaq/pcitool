#! /bin/bash

function pci {
    PCILIB_PATH=".."
    LD_LIBRARY_PATH="$PCILIB_PATH/pcilib" $PCILIB_PATH/pcitool/pci $*
}

rm -f images.raw


echo "Starting the grabber"
pci -g -o images.raw --run-time 3000000 &
pid=$!

usleep 1000000

for i in `seq 1 1000`; do
    echo "Trigger $i"
    pci --trigger
    usleep 100000
done

echo "Waiting grabber to finish"
wait $pid
