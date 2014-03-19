#! /bin/bash

location=/mnt/fast/
#location="./"
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

function reset {
    pci -r 0x9000 -s 256 > $1.status
    $TESTS_PATH/ipecamera/Reset_Init_all_reg_10bit.sh &> $1.reset 
}


$TESTS_PATH/ipecamera/cfg/Reset_Init_all_reg_10bit.sh &> /dev/null
$TESTS_PATH/ipecamera/frame.sh &> /dev/null
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
fpga_failures=0
cmosis_failures=0
frame_rate_failures=0

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
	    reset $output
	else
	    cur_failed=$(($cur_failed - 1))
	    cur_frames=`echo $cur_decoded | cut -f 2 -d ' '`
	    failed=$(($failed + $cur_failed))
	    frames=$(($frames + $cur_frames))
	    fpga_status=`pci -r 0x9054 | awk '{print $2;}' | cut -c 2`
	    cmosis_status=`pci -r 0x9050 | awk '{print $2;}' | cut -c 3-4`
	    if [ "$fpga_status" != "f" ]; then
		fpga_failures=$(($fpga_failures + 1))
		reset $output
	    elif [ "$cmosis_status" == "7d" ]; then
		cmosis_failures=$(($cmosis_failures + 1))
		reset $output
	    elif [ $cur_frames -lt 10 ]; then
		frame_rate_failures=$(($frame_rate_failures + 1))
		reset $output
	    elif [ $cur_failed -eq 0 ]; then
		rm -f $output
	    else
		reset $output
	    fi
	fi
    else
	failures=$(($failures + 1))
	reset $output
    fi

    echo "Frames: $frames, Failed Frames: $failed, Failed Exchanges: $failures, Failed Decodings: $decode_failures, FPGA Failures: $fpga_failures, CMOSIS Failures: $cmosis_failures, Low Frame Rate: $frame_rate_failures"
    iter=`expr $iter + 1`
done
