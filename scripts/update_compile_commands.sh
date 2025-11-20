#!/bin/bash -x

PWD= pwd

cp -r /var/lib/docker/volumes/build/_data/ .

rm -rf build/

mv _data/ build/

sed -i "s/\/app\/ThreadFuzzer/\/home\/jakob\/Documents\/uni\/doc\/project\/COTS_TESTS\/ThreadFuzzer/g" build/compile_commands.json
