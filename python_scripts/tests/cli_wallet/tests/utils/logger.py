import sys
import logging

log = logging.getLogger("Cli_wallet")
formater = '%(asctime)s [%(levelname)s] %(message)s'
stdh = logging.StreamHandler(sys.stdout)
stdh.setFormatter(logging.Formatter(formater))
log.addHandler(stdh)
log.setLevel(logging.INFO)