def isSubDict(response, pattern):
   for key in pattern.keys():
      if (not key in response) or (pattern[key] != response[key]):
         return False
   return True

COMPARATORS = { 'json_compare': isSubDict }
