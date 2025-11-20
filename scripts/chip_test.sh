#!/bin/bash -x

# goal: test the whole cycle automatically. That way we know if we break the Docker again
# service mdnsd stop
# ./third-party/ot-br-posix/script/server shutdown;
# ./third-party/ot-br-posix/script/server;

cat otbr-log/syslog | tail;
# sleep 1;

stty -F /dev/ttyUSB0 -hupcl -brkint -icrnl -imaxbel -opost -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke;

echo "############################# RUNNING THE LOOP ############################"

nb=0

while true
do
    echo $nb
    nb=$((nb + 1))

    ./third-party/ot-br-posix/script/server shutdown;
    ./third-party/ot-br-posix/script/server;
    sleep 0.5;

    ot-ctl factoryreset;
    sleep 1;
    ot-ctl dataset set active 0e08000000000001000000030000174a0300001035060004001fffe00708fd1e234fcca6183b0c0402a0f7f80102dead0208dead1111dead2222030d4a616b6f6273506c617950656e051011112233445566778899dead1111dead0410209f8ccb50f556da46166ef4fdcbed4a;
    ot-ctl ifconfig up;
    ot-ctl thread start;
    # sleep 5;


    # factoryreset the otbr
    ot-ctl factoryreset;
    sleep 4;
    ot-ctl dataset set active 0e08000000000001000000030000174a0300001035060004001fffe00708fd1e234fcca6183b0c0402a0f7f80102dead0208dead1111dead2222030d4a616b6f6273506c617950656e051011112233445566778899dead1111dead0410209f8ccb50f556da46166ef4fdcbed4a;
    ot-ctl dataset;

    # # factoryreset the device
    echo "ON" > /dev/ttyUSB0;
    sleep 10;
    echo "FR ON" > /dev/ttyUSB0;
    sleep 15;
    echo "FR OFF" > /dev/ttyUSB0;
    sleep 5;

    # start the otbr
    ot-ctl ifconfig up;
    ot-ctl thread start;

    # pair the device on the thread network
    ./connectedhomeip/out/chip-tool pairing ble-thread 6 hex:0e08000000000001000000030000174a0300001035060004001fffe00708fd1e234fcca6183b0c0402a0f7f80102dead0208dead1111dead2222030d4a616b6f6273506c617950656e051011112233445566778899dead1111dead0410209f8ccb50f556da46166ef4fdcbed4a 80049749 3070 --bypass-attestation-verifier true;

    # try playing around with it a bit...
    ./connectedhomeip/out/chip-tool generaldiagnostics read reboot-count 6 0 | tail;
    ot-ctl ifconfig down;
    echo "OFF" > /dev/ttyUSB0;
    sleep 5;
    ot-ctl ifconfig up;
    ot-ctl thread start;
    echo "ON" > /dev/ttyUSB0;
    sleep 5;
    # try playing around with it a bit...
    ./connectedhomeip/out/chip-tool generaldiagnostics read reboot-count 6 0 | tail;
    ot-ctl ifconfig down;
    echo "OFF" > /dev/ttyUSB0;
    sleep 3;
    ot-ctl ifconfig up;
    ot-ctl thread start;
    echo "ON" > /dev/ttyUSB0;
    sleep 5;
    # try playing around with it a bit...
    ./connectedhomeip/out/chip-tool generaldiagnostics read reboot-count 6 0 | tail;
    ot-ctl ifconfig down;
    echo "OFF" > /dev/ttyUSB0;
    sleep 3;
    ot-ctl ifconfig up;
    ot-ctl thread start;
    echo "ON" > /dev/ttyUSB0;
    sleep 5;
    # try playing around with it a bit...
    ./connectedhomeip/out/chip-tool generaldiagnostics read reboot-count 6 0 | tail;
    ot-ctl ifconfig down;
    echo "OFF" > /dev/ttyUSB0;
    sleep 3;
    ot-ctl ifconfig up;
    ot-ctl thread start;
    echo "ON" > /dev/ttyUSB0;
    sleep 5;
    # try playing around with it a bit...
    ./connectedhomeip/out/chip-tool generaldiagnostics read reboot-count 6 0 | tail;
    ot-ctl ifconfig down;
    echo "OFF" > /dev/ttyUSB0;
    sleep 3;
    ot-ctl ifconfig up;
    ot-ctl thread start;
    echo "ON" > /dev/ttyUSB0;
    sleep 5;

    # stop the otbr
    ot-ctl ifconfig down;
    ot-ctl thread stop;

    # stop the device
    echo "OFF" > /dev/ttyUSB0;
    sleep 1;


done
