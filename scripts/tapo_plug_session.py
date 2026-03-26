
import os
import argparse
import time
import asyncio
import re
from tapo import ApiClient
from dotenv import load_dotenv

class TAPO_Session():

    def __init__(self, username, password, ip_address):
        self.username = username
        self.password = password
        self.ip_address = ip_address

    async def init(self):
        client = ApiClient(self.username, self.password)
        self.device = await client.p110(self.ip_address)
        device_info = await self.device.get_device_info()
        print(f"Device info: {device_info.to_dict()}")

    async def power_on(self):
        await self.device.on()

    async def power_off(self):
        await self.device.off()

    async def restart(self, wait_time_s):
        await self.power_off()
        time.sleep(wait_time_s)
        await self.power_on()

async def main():

    load_dotenv()
    tapo_username = os.getenv("TAPO_LOGIN_EMAIL")
    tapo_password = os.getenv("TAPO_PASSWORD")
    ip_address = os.getenv("TAPO_IP_ADDRESS")

    if not tapo_username:
        print("Please set TAPO_LOGIN_EMAIL variable in .env file")
        return -1

    if not tapo_password:
        print("Please set TAPO_PASSWORD variable in .env file")
        return -1

    if not ip_address:
        print("Please set TAPO_IP_ADDRESS variable in .env file")
        return -1

    print("username", tapo_username)
    print("password", tapo_password)
    print("ip_addre", ip_address)

    ts = TAPO_Session(tapo_username, tapo_password, ip_address)
    await ts.init()

    # Create the argument parser
    parser = argparse.ArgumentParser(description="Script sending commands to a TAPO plug")

    # Add an argument for the pipe name
    parser.add_argument("-p", "--pipe_name", type=str, default="/tmp/tapo_pipe", help="The path to the named pipe (FIFO).")

    # Parse the command-line arguments
    args = parser.parse_args()

    PIPE_NAME = args.pipe_name

    # Create the named pipe if it doesn't exist
    if not os.path.exists(PIPE_NAME):
        os.mkfifo(PIPE_NAME)

    print(f"Listening for input on {PIPE_NAME}. Type 'exit' in another terminal to stop.")

    os.chmod(PIPE_NAME, 0o666)
    with open(PIPE_NAME, "r") as pipe:
        while True:
            time.sleep(1)
            line = pipe.readline().strip()  # Read from the pipe
            if not line:
                continue
            print(f"Line: {line}")
            if line.lower() == "exit":
                print("Exiting.")
                break
            restart_match = re.match(r"restart (\d+)", line, re.IGNORECASE)
            if restart_match:
                wait_time_s = int(restart_match.group(1))
                await ts.restart(wait_time_s)
            elif line == "on":
                await ts.power_on()
            elif line == "off":
                await ts.power_off()

    os.remove(PIPE_NAME)

if __name__ == "__main__":
    asyncio.run(main())


