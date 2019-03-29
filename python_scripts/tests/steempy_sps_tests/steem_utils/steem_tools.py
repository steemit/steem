# -*- coding: utf-8 -*-

import subprocess
import logging
import os
import time
import sys
import json
import datetime
import sys
import uuid

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./steem_tools.log"

MODULE_NAME = "Steem Tools Py"
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

def save_screen_cfg(cfg_file_name, log_file_path):
    """Creates a config file for screen command. In config file we configure logging path and interval.

    Args:
        cfg_file_name -- file name for screen config file,
        log_file_path -- path to log file.
    """
    with open(cfg_file_name, "w") as cfg:
        cfg.write("logfile {0}\n".format(log_file_path))
        cfg.write("deflog on\n")
        cfg.write("logfile flush 1\n")

def save_pid_file(pid_file_name, exec_name, port, start_time):
    """Creates PID file which indicate running keosd or nodeos process.

    Args:
        pid_file_name -- file name for pid file,
        exec_name -- name of the exectutable bound to this pid file,
        port -- port number for running executable,
        start_time -- execution start time.
    """
    with open(pid_file_name, "w") as pid_file:
            pid_file.write("{0}-{1}-{2}\n".format(exec_name, port, start_time))

def wait_for_string_in_file(log_file_name, string, timeout):
    """Blocks program execution until a given string is found in given file.

    Args:
        log_file_name -- path to scanned file,
        string -- sting to be found,
        timout -- block timeout in seconds, after this time exception will be raised.
    """
    logger.info("Waiting for string \"{}\" in file {}".format(string, log_file_name))
    step = 0.5
    to_timeout = 0.
    while True:
        time.sleep(step)
        to_timeout = to_timeout + step
        if to_timeout > timeout:
            msg = "Timeout during wait for string {0}".format(string)
            logger.error(msg)
            raise TimeoutError(msg)
        if not os.path.exists(log_file_name):
            continue
        with open(log_file_name, "r") as log_file:
            leave = False
            for line in log_file.readlines():
                if string in line:
                    leave = True
                    break
            if leave:
                break

def get_last_line_of_file(file_name):
    """Reads and returns last line of given file.

    Args:
        file_name -- path to a file.
    """
    last_line = ""
    with open(file_name, "r") as f:
        f.seek(-2, os.SEEK_END)
        while f.read(1) != b'\n':
            f.seek(-2, os.SEEK_CUR) 
        last_line = f.readline().decode()
    return last_line

def kill_process(pid_file_name, proc_name, ip_address, port):
    """Attempts to stop a process with given PID listening on port at ip_address. Process data is read from pid_file_name.

    Args:
        pid_file_name -- path to pid file,
        proc_name -- executable name,
        ip_address -- executable ip address,
        port -- executable port number.
    """
    logger.info("Terminating {0} process running on port {1}".format(proc_name, port))
    pids = []
    pid_name = None
    try:
        with open(pid_file_name, "r") as pid_file:
            pid_name = pid_file.readline()
            pid_name = pid_name.strip()
        if pid_name is not None:
            for line in os.popen("ps ax | grep " + proc_name + " | grep -v grep"):
                if pid_name in line:
                    line = line.strip().split()
                    pids.append(line[0])
            for pid in pids:
                for line in os.popen("ps --no-header --ppid {0}".format(pid)):
                    line = line.strip().split()
                    os.kill(int(line[0]), 2)
                os.kill(int(pid), 2)
            if os.path.exists(pid_file_name):
                os.remove(pid_file_name)
            logger.info("Done...")
        else:
            logger.warning("No such process: {0}".format(pid_name))
    except Exception as ex:
        logger.error("Process {0} cannot be killed. Reason: {1}".format(proc_name, ex))

def detect_process_by_name(proc_name, ip_address, port):
    """Checks if  process of given name runs on given ip_address and port.

    Args:
        proc_name -- process name,
        ip_address -- process ip address,
        port -- process port.
    """
    pids = []
    for line in os.popen("ps ax | grep " + proc_name + " | grep -v grep"):
        if ip_address in line and str(port) in line:
            line = line.strip().split()
            pids.append(line[0])
    if pids:
        msg = "{0} process is running on {1}:{2}. Please terminate that process and try again.".format(proc_name, ip_address, port)
        logger.error(msg)
        raise ProcessLookupError(msg)

def get_dynamic_global_properties(node_url, timeout = 240.):
    """Blocks program execution until proper response for get_info is received or timeout is reached.

    Args:
        timeout -- method will bloc until proper response for get_info is received or timeout is reached (default 60s),
    """
    import requests
    step = 3
    timeout_cnt = 0
    while True:
        query = {
          "jsonrpc" : "2.0", 
          "method" : "condenser_api.get_dynamic_global_properties", 
          "params" : [], 
          "id" : "{}".format(uuid.uuid4())
        }

        try:
            response = requests.post(node_url, json = query)
            if  response.status_code == 200:
                return response.json()
        except Exception as ex:
            logger.debug("Exception during get_dynamic_global_properties: {}".format(ex))
            pass
        time.sleep(step)
        timeout_cnt += step
        if timeout_cnt >= timeout:
            msg = "Timeout during get_dynamic_global_properties"
            logger.error(msg)
            raise TimeoutError(msg)


def wait_for_blocks_produced(block_count, node_url):
    logger.info("Waiting for {0} blocks to be produced...".format(block_count))

    global_properties = get_dynamic_global_properties(node_url)
    last_block_number = global_properties.get('result', None)
    if last_block_number is not None:
        last_block_number = last_block_number.get('head_block_number', None)
    logger.debug("Acquiring starting block number...")
    count = 0
    while last_block_number is None:
        global_properties = get_dynamic_global_properties(node_url)
        logger.debug("Global properties: {}".format(global_properties))
        last_block_number = global_properties.get('result', None)
        if last_block_number is not None:
            last_block_number = last_block_number.get('head_block_number', None)
        time.sleep(3)
        count += 1
        if count > (block_count + 10):
            msg = "Maximum tries exceeded"
            logger.error(msg)
            raise TimeoutError(msg)
    logger.debug("Got: {}".format(last_block_number))

    logger.debug("Acquiring current block number...")
    count = 0
    while True:
        global_properties = get_dynamic_global_properties(node_url)
        logger.debug("Global properties: {}".format(global_properties))
        curr_block_number = global_properties.get('result', 0)
        if curr_block_number != 0:
            curr_block_number = curr_block_number.get('head_block_number', 0)
        logger.debug("Got: {}".format(last_block_number))
        if curr_block_number - last_block_number > block_count:
            return
        time.sleep(3)
        count += 1
        if count > (block_count + 10):
            msg = "Maximum tries exceeded"
            logger.error(msg)
            raise TimeoutError(msg)


BLOCK_TYPE_HEADBLOCK = "within_reversible_block"
BLOCK_TYPE_IRREVERSIBLE = "within_irreversible_block"
def block_until_transaction_in_block(node_url, transaction_id, block_type = BLOCK_TYPE_HEADBLOCK, timeout = 60.):
    logger.info("Block until transaction_id: {0} is {1}".format(transaction_id, block_type))
    import time
    import requests
    step = 1.
    timeout_cnt = 0.
    while True:
        query = {
            "id" : "{}".format(uuid.uuid4()),
            "jsonrpc":"2.0", 
            "method":"transaction_status_api.find_transaction", 
            "params": {
                "transaction_id": transaction_id
            }
        }

        response = requests.post(node_url, json=query)
        transaction_status = response.get("status", None)
        if transaction_status is not None:
            if transaction_status == block_type:
                logger.info("Transaction id: {0} is {1}".format(transaction_id, block_type))
                return
            logger.info("Transaction id: {0} not {1}".format(transaction_id, block_type))
        time.sleep(step)
        timeout_cnt = timeout_cnt + step
        if timeout_cnt > timeout:
            msg = "Timeout reached during block_until_transaction_in_block"
            logger.error(msg)
            raise TimeoutError(msg)