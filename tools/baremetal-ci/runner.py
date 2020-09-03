import argparse
from datetime import datetime
import os
from pathlib import Path
import logging
from logging import getLogger
import tempfile
import threading
import time
import operator
import requests
import coloredlogs
from dotenv import load_dotenv
import serial

logger = getLogger("runner")
coloredlogs.install(level="INFO")
api = None
installer = None
rebooter = None

class API:
    def __init__(self, url, api_key):
        self.url = url
        self.api_key = api_key

    def request(self, method, path, **kwargs):
        kwargs.setdefault("headers", {})
        kwargs["headers"]["Authorization"] = f"bearer {self.api_key}"
        resp = requests.request(method, self.url + path, **kwargs)
        resp.raise_for_status()
        return resp

    def get(self, path, **kwargs):
        return self.request("get", path, **kwargs)

    def post(self, path, **kwargs):
        return self.request("post", path, **kwargs)

    def put(self, path, **kwargs):
        return self.request("put", path, **kwargs)

class GpioRebooter:
    def __init__(self):
        pass

    def reboot(self):
        pass

class CpInstaller:
    def __init__(self, install_path):
        self.dest = install_path

    def install(self, image):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp_path = Path(tmpdir) / "image"
            open(tmp_path, "wb").write(image)
            os.rename(tmp_path, self.dest)

newer_than = datetime.utcnow().timestamp()
def get_next_build(polling_interval):
    global newer_than
    logger.info("watching for new builds...")
    while True:
        new_builds = api.get("/api/builds", params={ "newer_than": newer_than }).json()
        if len(new_builds) > 0:
            newer_than = new_builds[0]["created_at"]
            return new_builds[0]
        time.sleep(polling_interval)

def update_run_status(run_id, new_status):
    api.put(f"/api/runs/{run_id}", json={ "status": new_status })


def run_build(args, build):
    logger.info(f"{build['id']}: Found a new build")
    run = {
        "status": "created",
        "runner_name": args.name,
        "build_id": build["id"],
    }
    run_id = api.post(f"/api/runs", json=run).json()["id"]

    logger.info(f"{build['id']}: Installing...")
    image = api.get(f"/api/builds/{build['id']}/image").content
    installer.install(image)

    logger.info(f"{build['id']}: Rebooting...")
    rebooter.reboot()
    update_run_status(run_id, "booting")

    log = ""
    started_at = time.time()
    timed_out = False
    with serial.Serial(args.serial_path, args.baudrate, timeout=1) as s:
        while True:
            log += "oh yeah\n"
            if started_at + args.timeout < time.time():
                timed_out = True
                break

            new_data = s.read().decode("utf-8", "backslashreplace")
            if len(new_data) == 0:
                continue

            log += new_data
            api.put(f"/api/runs/{run_id}/log", json={ "text": log })
            if "Passed all tests" in log:
                break

    status = "timeout" if timed_out else "finished"
    logger.info(f"{build['id']}: Finished the execution as status '{status}'")
    update_run_status(run_id, status)

def hearbeating(runner_name, machine):
    while True:
        api.put(f"/api/runners/{runner_name}", json={
            "name": runner_name,
            "machine": machine,
        })
        time.sleep(5 * 60)

def main():
    global api, installer, rebooter
    load_dotenv()
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True, help="The runner name.")
    parser.add_argument("--machine", required=True, help="The machine type.")
    parser.add_argument("--url", required=True, help="The BareMetal CI Server URL.")
    parser.add_argument("--timeout", type=int, default=30,
        help="The maximum running time in seconds for each run.")
    parser.add_argument("--api-key", help="The API Key.")
    parser.add_argument("--polling-interval", type=int, default=3)
    parser.add_argument("--install-by", choices=["cp"], required=True)
    parser.add_argument("--install-path",
        help="The destination path for the cp installer.")
    parser.add_argument("--reboot-by", choices=["gpio"], required=True)
    parser.add_argument("--serial-path", required=True)
    parser.add_argument("--baudrate", type=int, default=115200)
    args = parser.parse_args()

    api_key = os.environ.get("BAREMETALCI_API_KEY", args.api_key)
    api = API(args.url, api_key)

    if args.install_by == "cp":
        if not args.install_path:
            raise Exception("--install-path is not set")
        installer = CpInstaller(args.install_path)

    if args.reboot_by == "gpio":
        rebooter = GpioRebooter()

    logger.info("starting")
    threading.Thread(target=hearbeating, args=(args.name, args.machine), daemon=True).start()
    while True:
        build = get_next_build(args.polling_interval)
        run_build(args, build)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Ctrl-C")
