import os
import sys
import logging
import datetime

log = logging.getLogger("Cli_wallet")

def init_logger(_file_name):
    global log
    log.handlers = []
    formater = '%(asctime)s [%(levelname)s] %(message)s'
    stdh = logging.StreamHandler(sys.stdout)
    stdh.setFormatter(logging.Formatter(formater))
    log.addHandler(stdh)
    log.setLevel(logging.INFO)

    data = os.path.split(_file_name)
    if data[0]:
        path = data[0] + "/logs/"
    else:
        path = "./logs/"
    file = data[1]
    if path and not os.path.exists(path):
        os.makedirs(path)
    now = str(datetime.datetime.now())[:-7]
    now = now.replace(' ', '-')
    full_path = path+"/"+now+"_"+file
    if not full_path.endswith(".log"):
        full_path += (".log")
    fileh = logging.FileHandler(full_path)
    fileh.setFormatter(logging.Formatter(formater))
    log.addHandler(fileh)

