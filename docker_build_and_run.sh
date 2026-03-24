pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
docker build --pull --progress=plain -t thread_fuzzer:latest . ;
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
		-v /tmp/tapo_pipe:/tmp/tapo_pipe \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "1";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "2";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "3";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "4";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "5";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "6";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "7";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "8";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "9";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "10";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "11";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "12";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "13";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "14";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "15";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "16";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "17";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "18";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "19";
docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
		-v /var/cache:/var/cache \
		-v /var/crash:/var/crash \
		-v /var/dbus:/var/dbus \
		-v /var/lib:/var/lib \
		-v /var/log:/var/log \
		-v /var/metrics:/var/metrics \
		-v /var/opt:/var/opt \
		-v /var/tmp:/var/tmp \
		-v /var/usr:/var/usr \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
		--cap-add=SYS_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer;
pkill avahi-daemon;
pkill otbr-agent;
pkill mdnsd;
echo "done 20"
