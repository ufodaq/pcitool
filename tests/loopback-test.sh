#! /bin/bash

TESTS_PATH="`dirname \"$0\"`" 
TESTS_PATH="`( cd \"$TESTS_PATH\" && pwd )`"

function pci {
    PCILIB_PATH=$TESTS_PATH/..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

function compare {
    PCILIB_PATH=$TESTS_PATH/..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/apps/compare_to_value $*
}

#size=`expr 1024 "*" 1024`
size=`expr 1024 "*" 1`
multiplier=2
wait=0

/root/pcitool/tests/frame.sh &> /dev/null
rm -f bench.out

pci --stop-dma dma1
pci --start-dma dma1

pci -r dma1 -s 16777216 --multipacket -o /dev/null &> /dev/null

pci -r dma1 -s 1024 -o /dev/null | grep -i "Error (62)" &> /dev/null
if [ $? -ne 0 ]; then
    echo "There is data on dma..."
    exit
fi

failed=0
send=0
errors=0
err_num=0

print_iters=`expr 2 + 1024 '*' 100 / $size`

read_size=`expr $multiplier '*' $size`
echo "Starting..."
i=1

pci -w 0x9040 0x201

while [ 1 ]; do
    if [ $wait -gt 0 ]; then
	pci -w 0x9040 0x1
    fi
    
    pci -w dma1 -s $size "*0x$i"
    rm -f /tmp/camera-test.out
    pci -r dma1 --wait --multipacket -s $read_size -o /tmp/camera-test.out -t 1000000 &> /dev/null
    if [ $wait -gt 0 ]; then
	wrdone=0
	while [ $wrdone -eq 0 ]; do
#	    pci --list-dma-engines
	    pci --list-dma-engines | grep "DMA1 S2C" | grep "SD" #&> /dev/null
	    wrdone=$?
	done
	pci -w 0x9040 0x201
    fi

    res=`compare /tmp/camera-test.out  $read_size "$i" 6 2 6`
    if [ $? -eq 0 ]; then
	err_cnt=`echo $res | cut -f 1 -d ' '`
	if [ "$err_cnt" -ne 0 ]; then
	    pci -r dma1 --wait --multipacket -o /tmp/camera-test.out -t 1000000 &> /dev/null
	    cp /tmp/camera-test.out /tmp/camera-test.out.$err_num
	    err_num=$(($err_num + 1))
	fi
	byte_cnt=`echo $res | cut -f 3 -d ' '`
	send=$(($send + $byte_cnt * 4))
	errors=$(($errors + $err_cnt * 4))
    else 
	failed=$(($failed + 1))
    fi
    
    i=$((i + 1))
    if [ $i -eq $print_iters ]; then
	echo "Data send: $send bytes, Errors: $errors bytes, Failed exchanges: $failed"
#	pci -r 0x9070 -s 4	
	i=1
    fi
done
