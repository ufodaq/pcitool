#! /bin/bash

BAR=0
USE=1
ITERATIONS=2
BUFFERS=16

function pci {
    PCILIB_PATH=`pwd`/..
    LD_LIBRARY_PATH="$PCILIB_PATH" $PCILIB_PATH/pci $*
}


function reset {
    pci -b $BAR -w 0 1
    usleep 1000
    pci -b $BAR -w 0 0
    pci -b $BAR -w 4 0
}

function read_cfg {
#    echo $1 1>&2
    pci -a config -r 0x$1 | awk '{ print $2; }'
}

function parse_config {
    info=0x`pci -b $BAR -r 0 | awk '{ print $2; }'`
    model=`printf "%X" $((info>>24))`
    if [ $model -eq 14 ]; then
	model="Xilinx Virtex-6"
    else
	model="Xilinx $model"
    fi
    version=$(((info >> 8) & 0xFF))
    data_width=$((16 * (2 ** ((info >> 16) & 0xF))))
    
    echo "$model, build $version, $data_width bits"


    next=`read_cfg 34 | cut -c 7-8`

    while [ $next -ne 0 ]; do
	cap=`read_cfg $next`
	capid=`echo $cap | cut -c 7-8`
	if [ $capid -eq 10 ]; then
	    addr=`printf "%X" $((0x$next + 12))`
	    pcie_link1=`read_cfg $addr`
	    addr=`printf "%X" $((0x$next + 16))`
	    pcie_link2=`read_cfg $addr`

	    link_speed=$((((0x$pcie_link2 & 0xF0000) >> 16)))
	    link_width=$((((0x$pcie_link2 & 0x3F00000) >> 20)))

	    dev_link_speed=$((((0x$pcie_link1 & 0xF))))
	    dev_link_width=$((((0x$pcie_link1 & 0x3F0) >> 4)))
	fi
	next=`echo $cap | cut -c 5-6`
    done

    echo "Link: PCIe gen$link_speed x$link_width"
    if [ $link_speed -ne $dev_link_speed -o $link_width -ne $dev_link_width ]; then
	echo " * But device capable of gen$dev_link_speed x$dev_link_width"
    fi
    
    info=0x`read_cfg 40`
    max_tlp=$((2 ** (5 + ((info & 0xE0) >> 5))))
    echo "TLP: 32 dwords (transfering 32 TLP per request)"
    if [ $max_tlp -ne 32 ]; then
	echo " * But device is able to transfer TLP up to $max_tlp bytes"
    fi
    
    # 2500 MT/s, but PCIe gen1 and gen2 uses 10 bit encoding
    speed=$((link_width * link_speed * 2500 / 10))
}

reset
parse_config

pci --enable-irq
pci --acknowledge-irq

pci --free-kernel-memory $USE
pci --alloc-kernel-memory $USE --type c2s -s $BUFFERS
bus=`pci --list-kernel-memory 00100001 | awk '{ print $4; }' | grep 00`
#ptr=`pci --list-kernel-memory 00100001 | awk '{ print $2; }' | grep 00`

# TLP size
pci -b $BAR -w 0x0C 0x20
# TLP count
pci -b $BAR -w 0x10 0x20
# Data
pci -b $BAR -w 0x14 0x13131313

dmaperf=0
for i in `seq 1 $ITERATIONS`; do
  for addr in $bus; do 
    pci -b $BAR -w 0x08 0x$addr

#Trigger
    pci -b $BAR -w 0x04 0x01
    pci --wait-irq
#    pci -b $BAR -w 0x04 0x00

    status=`pci -b $BAR -r 0x04 | awk '{print $2; }' | cut -c 5-8`
    if [ $status != "0101" ]; then
	echo "Read failed, invalid status: $status"
    fi

    dmaperf=$((dmaperf + 0x`pci -b $BAR -r 0x28 | awk '{print $2}'`))
    reset
  done
done

pci --free-kernel-memory $USE
pci --disable-irq

echo
# Don't ask me about this formula
echo "Performance reported by FPGA: $((4096 * BUFFERS * ITERATIONS * $speed / $dmaperf / 8)) MB/s"

#pci -b $BAR  -r 0 -s 32
