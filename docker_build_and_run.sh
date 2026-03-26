#!/bin/bash
for i in {01..01}; do
	echo $i
	pkill avahi-daemon
	pkill otbr-agent
	pkill mdnsd
	sudo docker build --pull --progress=plain -t thread_fuzzer:latest .
	sudo docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
                -v /var:/var \
                -v /proc:/proc \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --cap-add=NET_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer
	sleep 10
done
