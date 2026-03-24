#!/bin/bash

function shutdown()
{
    echo "Shutting down"
    sudo ./third-party/ot-br-posix/script/server shutdown
    exec /bin/bash
    exit 0
}

trap shutdown TERM INT

set -m

echo "RUNNING DOCKER ENTRY POINT SCRIPT!!"

echo "[INFO]: create dummy network interface"
sudo ./scripts/dummy_interface.sh

echo "[INFO]: hooking dummy interface up to otbr"
sed -i "s/eth0/dummy0/g" /etc/default/otbr-agent
sed -i "s/wlan0/dummy0/g" /etc/default/otbr-agent

echo "[INFO]: starting custom rsyslog"
rsyslogd -n -f syslog.conf &

while [[ ! -S /dev/log ]]; do
    sleep 1
    echo "[ERROR]: rsyslog not active yet, waiting a second..."
done

echo "[INFO]: testing by calling logger"
logger "[INFO] test, test, this is a test..."

echo "[INFO]: syslog >>>"
cat /app/ThreadFuzzer/otbr-log/syslog | grep INFO
echo "[INFO]: <<< syslog"

# build the project here, makes incremental changes easier
# when sharing build/ as a volume to the container
cd build && cmake .. && make -j3;
cd ..

# echo "[INFO]: starting otbr"
# ./third-party/ot-br-posix/script/server
# ./build/ThreadFuzzer configs/Fuzzing_Settings/phys_main_config.json configs/Fuzzing_Strategies/reboot_cnt_config.json
# echo "[INFO]: checking syslog"
# tail -f otbr-log/syslog &
# wait $!

# echo "[INFO]: if all went right, otbr should now be running in the background"
echo "[INFO]: dropping into shell";

# in case we're testing the NANOLEAF lightbulb
# chmod 777 /tmp/tapo_pipe && chown root /tmp/tapo_pipe;

# exec /bin/bash
# here we 
exec /bin/bash -c ./run_experiment.sh && exit
