# -*- coding: utf-8 -*-

import subprocess
import logging
import os
import time
import sys
import json
import datetime

from .steem_tools import *

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./steem_runner.log"

MODULE_NAME = "STEEM Runner Py"
logger = logging.getLogger(MODULE_NAME)
logger.setLevel(LOG_LEVEL)

ch = logging.StreamHandler(sys.stdout)
ch.setLevel(LOG_LEVEL)
ch.setFormatter(logging.Formatter(LOG_FORMAT))

fh = logging.FileHandler(MAIN_LOG_PATH)
fh.setLevel(LOG_LEVEL)
fh.setFormatter(logging.Formatter(LOG_FORMAT))

if not logger.hasHandlers():
    logger.addHandler(ch)
    logger.addHandler(fh)

class SteemNode(object):
    def __init__(self, steem_executable, working_dir, config_src_path):
        self.steem_executable = steem_executable
        self.working_dir = working_dir
        self.config_src_path = config_src_path

        from shutil import rmtree, copy
        # remove old data from node
        if os.path.exists(self.working_dir):
            rmtree(self.working_dir)
        os.makedirs(self.working_dir)
        # copy config file to working dir
        copy(self.config_src_path, self.working_dir + "/config.ini")

        self.steem_config = self.parse_node_config_file(self.working_dir + "/config.ini")
        self.ip_address, self.port = self.steem_config["webserver-http-endpoint"][0].split(":")
        self.ip_address = "http://{}".format(self.ip_address)
        self.node_running = False

    def get_from_config(self, key):
        return self.steem_config.get(key, None)

    def get_node_url(self):
        return "{}:{}/".format(self.ip_address, self.port)

    def is_running(self):
        return self.node_running

    def parse_node_config_file(self, config_file_name):
        ret = dict()
        lines = None
        with open(config_file_name, "r") as f:
            lines = f.readlines()

        for line in lines:
            proc_line = line.strip()
            if proc_line:
                if proc_line.startswith("#"):
                    continue
                k, v = proc_line.split("=", 1)
                k = k.strip()
                v = v.strip()
                if k in ret:
                    ret[k].append(v)
                else:
                    ret[k] = [v]
        return ret

    def run_steem_node(self, additional_params = [], wait_for_blocks = True):
        detect_process_by_name("steem", self.ip_address, self.port)

        logger.info("*** START NODE at {0}:{1} in {2}".format(self.ip_address, self.port, self.working_dir))

        parameters = [
            self.steem_executable,
            "-d",
            self.working_dir,
            "--advanced-benchmark",
            "--sps-remove-threshold",
            "-1"
        ]

        parameters = parameters + additional_params
        
        self.pid_file_name = "{0}/run_steem-{1}.pid".format(self.working_dir, self.port)
        current_time_str = datetime.datetime.now().strftime("%Y-%m-%d")
        log_file_name = "{0}/{1}-{2}-{3}.log".format(self.working_dir, "steem", self.port, current_time_str)
        screen_cfg_name = "{0}/steem_screen-{1}.cfg".format(self.working_dir, self.port)

        save_screen_cfg(screen_cfg_name, log_file_name)
        screen_params = [
            "screen",
            "-m",
            "-d",
            "-L",
            "-c",
            screen_cfg_name,
            "-S",
            "{0}-{1}-{2}".format("steem", self.port, current_time_str)
        ]

        parameters = screen_params + parameters
        logger.info("Running steemd with command: {0}".format(" ".join(parameters)))
        
        try:
            subprocess.Popen(parameters)
            save_pid_file(self.pid_file_name, "steem", self.port, current_time_str)
            if wait_for_blocks:
                wait_for_blocks_produced(2, "{}:{}".format(self.ip_address, self.port))
            else:
                wait_for_string_in_file(log_file_name, "start listening for ws requests", 240.)
            self.node_running = True
            logger.info("Node at {0}:{1} in {2} is up and running...".format(self.ip_address, self.port, self.working_dir))
        except Exception as ex:
            logger.error("Exception during steemd run: {0}".format(ex))
            kill_process(self.pid_file_name, "steem", self.ip_address, self.port)
            self.node_running = False


    def stop_steem_node(self):
        logger.info("Stopping node at {0}:{1}".format(self.ip_address, self.port))
        kill_process(self.pid_file_name, "steem", self.ip_address, self.port)
        self.node_running = False


if __name__ == "__main__":
    node = SteemNode("/home/dariusz-work/Builds/steem/programs/steemd/steemd", "/home/dariusz-work/steem-data", "./resources/config.ini.in")
    node.run_steem_node()
    node.stop_steem_node()
