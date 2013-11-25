#! /bin/bash

#location=/mnt/fast/
location="./"
duration=10000000
wait_frame=1000000

TESTS_PATH="`dirname \"$0\"`" 
TESTS_PATH="`( cd \"$TESTS_PATH\" && pwd )`"

function pci {
    PCILIB_PATH=$TESTS_PATH/..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}

function stop {
    usleep $duration
    pci -w control 0x201 &> /dev/null
}

/root/pcitool/tests/frame.sh &> /dev/null
rm -f bench.out

pci --stop-dma
pci --start-dma dma1


pci -r dma1 -s 16777216 --multipacket -o /dev/null &> /dev/null

pci -r dma1 -s 1 | grep -i "Error (62)" &> /dev/null
if [ $? -ne 0 ]; then
    echo "There is data on dma..."
    exit
fi

echo "Starting ... "

decode_failures=0
failures=0
failed=0
frames=0

iter=0
while [ 1 ]; do
    pci -w control 0xa01 &> /dev/null
    stop &

    output="$location/test$iter.out"
    rm -f $output
    pci -r dma1 -o $output  --wait --multipacket -t $wait_frame &> /dev/null

    killall -9 usleep &> /dev/null
    usleep 100000
    pci -w control 0x201 &> /dev/null
    
    if [ -f $output ]; then
	result=`ipedec -d -v --continue $output 2>&1 | grep -iE "failed|decoded"`
	cur_failed=`echo $result | wc -l`
	cur_decoded=`echo $result | tail -n 1 | grep -i decoded`
	if [ $? -ne 0 -o $cur_failed -eq 0 ]; then
	    ipedec -d -v --continue $output > $output.decode
	    decode_failures=$(($decode_failures + 1))
	else
	    cur_failed=$(($cur_failed - 1))
	    cur_frames=`echo $cur_decoded | cut -f 2 -d ' '`
	    failed=$(($failed + $cur_failed))
	    frames=$(($frames + $cur_frames))
	    if [ $cur_failed -eq 0 ]; then
		rm -f $output
	    fi
	fi
    else
	failures=$(($failures + 1))
    fi

    echo "Frames: $frames, Failed Frames: $failed, Failed Exchanges: $failures, Failed Decodings: $decode_failures"
    iter=`expr $iter + 1`
done
