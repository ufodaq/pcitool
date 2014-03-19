#! /bin/bash

SCRIPT_PATH="`dirname \"$0\"`" 
SCRIPT_PATH="`( cd \"$TESTS_PATH\" && pwd )`"
PCILIB_PATH=${SCRIPT_PATH%/tests/*}

function pci {
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

function strip_bad_values {
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/apps/heb_strip_bad_values $*
}

function request_data {
    $PCILIB_PATH/tests/heb/debug/request_data.sh $*
}

while [ 1 ]; do
    from=`pci --list-dma-engines | grep C2S | sed -s 's/\s\+/ /g' | cut -d ' ' -f 6`
    to=`pci --list-dma-engines | grep C2S | sed -s 's/\s\+/ /g' | cut -d ' ' -f 8`

    if [ $from -gt $to ]; then
	buffers="`seq $from 255` `seq 0 $to`"
    else
	buffers=`seq $from $to`
    fi

    echo $buffers

    rm data.out
    for i in $buffers; do
	pci --read-dma-buffer dma1r:$i -o data.out
    done


    error=`strip_bad_values data.out | head -n 1 | cut -f 1 -d ':'`
    if [ $error != "0x1140" ]; then
	echo "Problems found"
	exit
    else
	echo "Fine"
	request_data
    fi
done
