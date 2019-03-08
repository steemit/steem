#!/usr/bin/python3

import requests
import json
import logging
import sys
import uuid

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_rpc_api_test.log"

MODULE_NAME = "SPS API Tester"
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

def get_random_id():
  return str(uuid.uuid4())

def list_voter_proposals(voter, order_by, order_direction, limit, active):
  payload = {
    "jsonrpc" : "2.0",
    "id" : get_random_id(),
    "method" : "sps_api.list_voter_proposals", 
    "params" : {
      "voter" : voter,
      "order_by" : order_by, 
      "order_direction" : order_direction,
      "limit" : limit,
      "active" : active
    }
  }
  ret = json.dumps(payload)
  logger.info("New payload: {}".format(ret))
  return ret

def find_proposals(id_set):
  payload = {
    "jsonrpc" : "2.0",
    "id" : get_random_id(),
    "method" : "sps_api.find_proposals", 
    "params" : {
      "id_set" : id_set
    }
  }
  ret = json.dumps(payload)
  logger.info("New payload: {}".format(ret))
  return ret

def list_proposals(start, order_by, order_direction, limit, active):
  payload = {
    "jsonrpc" : "2.0",
    "id" : get_random_id(),
    "method" : "sps_api.list_proposals", 
    "params" : {
      "start" : start,
      "order_by" : order_by, 
      "order_direction" : order_direction,
      "limit" : limit,
      "active" : active
    }
  }
  ret = json.dumps(payload)
  logger.info("New payload: {}".format(ret))
  return ret

def condenser_list_proposals(start, order_by, order_direction, limit, active):
  payload = {
    "jsonrpc" : "2.0",
    "id" : get_random_id(),
    "method" : "condenser_api.list_proposals", 
    "params" : [start, order_by, order_direction, limit, active]
  }
  ret = json.dumps(payload)
  logger.info("New payload: {}".format(ret))
  return ret

def condenser_list_voter_proposals(voter, order_by, order_direction, limit, active):
  payload = {
    "jsonrpc" : "2.0",
    "id" : get_random_id(),
    "method" : "condenser_api.list_voter_proposals", 
    "params" : [voter, order_by, order_direction, limit, active]
  }
  ret = json.dumps(payload)
  logger.info("New payload: {}".format(ret))
  return ret

def condenser_find_proposals(id_set):
  payload = {
    "jsonrpc" : "2.0",
    "id" : get_random_id(),
    "method" : "condenser_api.find_proposals", 
    "params" : [id_set]
  }
  ret = json.dumps(payload)
  logger.info("New payload: {}".format(ret))
  return ret

def run_test(test_name, expected_result, url, payload):
  logger.info("Running test: {}".format(test_name))
  try:
    response = requests.request("POST", url, data = payload)
    logger.info("Response: {}".format(response.json()))
  except Exception as ex:
    logger.error("Exception during processing request: {0}".format(ex))

if __name__ == '__main__':
  logger.info("Performing SPS RPC API plugin tests")
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument("--node-ip", help = "IP address of steem node", default = "http://127.0.0.1", dest = "node_ip")
  parser.add_argument("--node-port", help = "Steem node port", default = 8751, dest = "node_port")

  args = parser.parse_args()

  url = "{0}:{1}".format(args.node_ip, args.node_port)
  logger.info("Using node at: {}".format(url))

  logger.info("Running SPS RPC API tests")
  payload = find_proposals([1234, 2, 3, 4])
  run_test("Basic find_proposal test", None, url, payload)

  payload = list_proposals("blocktrades", "by_creator", "direction_ascending", 1, 1)
  run_test("Basic list_proposals test", None, url, payload)

  payload = list_voter_proposals("blocktrades", "by_creator", "direction_ascending", 1, 1)
  run_test("Basic list_voter_proposals test", None, url, payload)

  logger.info("Running SPS API tests via condeser API")
  payload = condenser_find_proposals([1234, 2, 3, 4])
  run_test("Basic find_proposal test", None, url, payload)

  payload = condenser_list_proposals("blocktrades", "by_creator", "direction_ascending", 1, 1)
  run_test("Basic list_proposals test", None, url, payload)

  payload = condenser_list_voter_proposals("blocktrades", "by_creator", "direction_ascending", 1, 1)
  run_test("Basic list_voter_proposals test", None, url, payload)

