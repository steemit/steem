def dict_contain(response, pattern):
   for key in pattern.keys():
      if (not key in response) or (pattern[key] != response[key]):
         return False
   return True

def list_contain(response, pattern):
   for item in pattern:
      if item not in response:
         return False
   return True

def contain(response, pattern):
   if type(response) != type(pattern):
      return False

   if type(response) == 'dict':
      return dict_contain(response, pattern)
   if type(response) == 'list':
      return list_contain(response, pattern)
   if type(response) == 'str':
      return pattern in response
   # all other types
   return pattern == response

COMPARATORS = { 'json_compare': contain }
