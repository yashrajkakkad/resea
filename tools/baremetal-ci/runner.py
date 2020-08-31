import argparse
import os
import logging
from logging import getLogger
import tempfile
import time
import operator
import requests
import coloredlogs

logger = getLogger("runner")
coloredlogs.install(level="INFO")
api = None
installer = None
rebooter = None

class API:
    def __init__(self, url, api_key):
        self.url = url
        self.api_key = api_key

    def get(self, path, **kwargs):
        headers = kwargs.get("headers", {})
        headers["Authorization"] = f"bearer #{self.api_key}"
        return requests.get(self.url + path, **kwargs)

    def post(self, path, **kwargs):
        headers = kwargs.get("headers", {})
        headers["Authorization"] = f"bearer #{self.api_key}"
        return requests.post(self.url + path, **kwargs)

    def put(self, path, **kwargs):
        headers = kwargs.get("headers", {})
        headers["Authorization"] = f"bearer #{self.api_key}"
        return requests.put(self.url + path, **kwargs)

class GpioRebooter:
    def __init__(self):
        pass

    def reboot(self):
        pass

class CpInstaller:
    def __init__(self, install_path):
        self.dest = install_path

    def install(self):
        with tempfile.NamedTemporaryFile as tmp:
            open(tmp.name, "wb").write(image)
            os.rename(tmp.name, self.dest)

last_build_id = 0
def get_next_build(polling_interval):
    global last_build_id
    logger.info("watching for new builds...")
    while True:
        builds = api.get("/api/builds").json()["builds"]
        new_builds = list(filter(lambda build: build["id"] > last_build_id,
            sorted(builds, key=operator.itemgetter("id"))))
        if len(new_builds) > 0:
            return new_builds[0]
        time.sleep(polling_interval)

def update_run_status(run_id, new_status):
    api.put(f"/api/runs/{run_id}", json={ "status": new_status })

def run_build(build):
    image = api.get(f"/api/builds/{build['id']}/image").content

    run_id = api.post(f"/api/runs").json()["id"]
    installer.install(image)

    # start_reading_serial()
    rebooter.reboot()

    update_run_status(run_id, "booting")

def main():
    global api, installer, rebooter
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", required=True)
    parser.add_argument("--api-key")
    parser.add_argument("--polling-interval", type=int, default=3)
    parser.add_argument("--install-by", choices=["cp"])
    parser.add_argument("--install-path",
        help="The destination path for the cp installer.")
    parser.add_argument("--reboot-by", choices=["gpio"])
    args = parser.parse_args()

    # register_runner()

    api_key = os.environ.get("API_KEY", args.api_key)
    api = API(args.url, api_key)

    if args.install_by == "cp":
        if args.install_path:
            raise "--install-path is not set"
        installer = CpInstaller(args.install_path)

    if args.reboot_by == "gpio":
        rebooter = GpioRebooter()

    while True:
        build = get_next_build(args.polling_interval)
        run_build(build)

if __name__ == "__main__":
    main()
