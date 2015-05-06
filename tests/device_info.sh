#! /bin/bash

BAR=0

function pci {
    PCILIB_PATH=`pwd`/..
    LD_LIBRARY_PATH="$PCILIB_PATH/pcilib" $PCILIB_PATH/pcitool/pci $*
}


function read_cfg {
    pci -a config -r 0x$1 | awk '{ print $2; }' | sed -e 's/\s*//g' -e '/^\s*$/d'
}

function parse_config {
    info=0x`pci -b $BAR -r 0 | awk '{ print $2; }' | sed -e 's/\s*//g' -e '/^\s*$/d'`
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
#    next=`printf "%u" $next`

    while [ $((0x$next)) -ne 0 ]; do
	cap=`read_cfg $next`
	capid=`echo $cap | cut -c 7-8`
	if [ $capid -eq 10 ]; then
	    addr=`printf "%X" $((0x$next + 4))`
	    device_capabilities=`read_cfg $addr`

	    addr=`printf "%X" $((0x$next + 8))`
	    device_control=`read_cfg $addr`

	    addr=`printf "%X" $((0x$next + 12))`
	    pcie_link1=`read_cfg $addr`
	    addr=`printf "%X" $((0x$next + 16))`
	    pcie_link2=`read_cfg $addr`

	    link_speed=$((((0x$pcie_link2 & 0xF0000) >> 16)))
	    link_width=$((((0x$pcie_link2 & 0x3F00000) >> 20)))

	    dev_link_speed=$((((0x$pcie_link1 & 0xF))))
	    dev_link_width=$((((0x$pcie_link1 & 0x3F0) >> 4)))
	    
	    max_payload=$(((1 << ((0x$device_capabilities & 0x07) + 7))))
	    dev_payload=$(((1 << (((0x$device_capabilities >> 5) & 0x07) + 7))))
	fi
	next=`echo $cap | cut -c 5-6`
    done

    echo "Link: PCIe gen$link_speed x$link_width"
    if [ $link_speed -ne $dev_link_speed -o $link_width -ne $dev_link_width ]; then
	echo " * But device capable of gen$dev_link_speed x$dev_link_width"
    fi
    
    echo "Payload: $dev_payload"
    if [ $dev_payload -ne $max_payload ]; then
	echo " * But device capable of $max_payload"
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

parse_config
