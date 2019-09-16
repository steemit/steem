#!/usr/bin/env python3
import os
import sys
import glob
import argparse
import datetime
import subprocess

from tests.utils.cmd_args import parser
from tests.utils.logger import log

test_args = []
summary_file_name = "summary.txt"

def check_subdirs(_dir):
  error = False
  tests = sorted(glob.glob(_dir+"/*.py"))
  if tests:
    for test in tests:
      root_error = run_script(test)
      if root_error:
        error = root_error
  return error


def run_script(_test, _multiplier = 1, _interpreter = None ):
  try:
      with open(summary_file_name, "a+") as summary:
        interpreter = _interpreter if _interpreter else "python3"
        ret_code = subprocess.call(interpreter + " " + _test + " " + test_args, shell=True)
        if ret_code == 0:
            summary.writelines("Test `{0}` passed.\n".format(_test))
            return False
        else:
            summary.writelines("Test `{0}` failed.\n".format(_test))
            return True
  except Exception as _ex:
    log.exception("Exception occures in run_script `{0}`".format(str(_ex)))
    return True


if __name__ == "__main__":
    if os.path.isfile(summary_file_name):
        os.remove(summary_file_name)
    with open(summary_file_name, "a+") as summary:
        summary.writelines("Cli wallet test started at {0}.\n".format(str(datetime.datetime.now())[:-7]))
    args = parser.parse_args()
    for key, val in args.__dict__.items():
        if val :
            test_args.append("--"+key.replace("_","-")+ " ")
            test_args.append(val)
    test_args = " ".join(test_args)
    try:
        error = True
        if os.path.isdir("./tests"):
            error = check_subdirs("./tests")

    except Exception as _ex:
        log.exception("Exception occured `{0}`.".format(str(_ex)))
        error = True
    finally:
        if error:
            log.error("At least one test has faild. Please check summary.txt file.")
            exit(1)
        else:
            log.info("All tests pass.")
            exit(0)
