#!/usr/bin/env python3
import sys
import json
import socket

class JSONSocket(object):
  """
  Class encapsulates socket object with special handling json rpc.
  timeout is ignored now - nonblicking socket not supported.
  """
  def __init__(self, host, port, path, timeout=None):
    """
    host in form [http[s]://]<ip_address>[:port]
    if port not in host must be spefified as argument
    """
    self.__sock = None
    if host.find("http://") == 0 or host.find("https://") == 0:
      host = host[host.find("//")+2 : len(host)]
    _host = host
    if port != None:
      host = host + ":" + str(port)
    else:
      colon_pos = host.rfind(":")
      _host = host[0 : colon_pos]
      port = int(host[colon_pos+1 : len(host)])
    self.__fullpath = host + path
    self.__host = bytes(host, "utf-8")
    self.__path = bytes(path, "utf-8")
    #print("<host>: {}; <port>: {}".format(_host, port))
    self.__sock = socket.create_connection((_host, port))
    self.__sock.setblocking(True)
    #self.__sock.settimeout(timeout)
    self.__head = b"POST " + self.__path + b" HTTP/1.0\r\n" + \
                  b"HOST: " + self.__host + b"\r\n" + \
                  b"Content-type: application/json\r\n"

  def get_fullpath(self):
    return self.__fullpath

  def request(self, data=None, json=None):
    """
    data - complete binary form of json request (ends with '\r\n'
    json - json request as python dict
    
    return value in form of json response as python dict
    """
    if data == None:
      data = bytes(json.dumps(json), "utf-8") + b"\r\n"
    length = bytes(str(len(data)), "utf-8")
    request = self.__head + \
              b"Content-length: " + length + b"\r\n\r\n" + \
              data
    #print("request:", request.decode("utf-8"))
    self.__sock.sendall(request)
    #self.__sock.send(request)
    status, response = self.__read()
    #if response == {}:
    #  print("response is empty for request:", request.decode("utf-8"))
    return status, response
    
  def __call__(self, data=None, json=None):
    return self.request(data, json)

  def __read(self):
    response = ''
    binary = b''
    while True:
      temp = self.__sock.recv(4096)
      if not temp: break
      binary += temp

    response = binary.decode("utf-8")

    if response.find("HTTP") == 0:
      response = response[response.find("\r\n\r\n")+4 : len(response)]
      if response and response != '':
        r = json.loads(response)
        if 'result' in r:
          return True, r
        else:
          return False, r
      
    return False, {}
    
  def __del__(self):
    if self.__sock:
      self.__sock.close()

      
def steemd_call(host, data=None, json=None, max_tries=10, timeout=0.1):
  """
  host - [http[s]://<ip_address>:<port>
  data - binary form of request body, if missing json object should be provided (as python dict/array)
  """
#  try:
#    jsocket = JSONSocket(host, None, "/rpc", timeout)
#  except:
#    print("Cannot open socket for:", host)
#    return False, {}
    
  for i in range(max_tries):
    try:
       jsocket = JSONSocket(host, None, "/rpc", timeout)
    except:
       type, value, traceback = sys.exc_info()
       print("Error: {}:{} {} {}".format(1, type, value, traceback))
       print("Error: Cannot open JSONSocket for:", host)
       continue
    try:
      status, response = jsocket(data, json)
      if status:
        return status, response
    except:
       type, value, traceback = sys.exc_info()
       print("Error: {}:{} {} {}".format(1, type, value, traceback))
       print("Error: JSONSocket request failed for: {} ({})".format(host, data.decode("utf-8")))
       continue
  else:
    return False, {}
