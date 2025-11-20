#!/bin/bash -x

sudo modprobe dummy

sudo ip link add dummy0 type dummy

ip link show dummy0

sudo ifconfig dummy0 hw ether C8:D7:4A:4E:47:50

sudo ip addr add 192.168.1.100/24 brd + dev dummy0 label dummy0:0

sudo ip link set dev dummy0 up

ip a

