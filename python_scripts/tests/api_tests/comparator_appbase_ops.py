def isAppbaseOperations(appbase_response, pre_appbase_pattern):
   #print(type(appbase_response));
   #print(type(pre_appbase_pattern));
   if ("ops" in appbase_response == False):
      return False

   appbase_ops = appbase_response["ops"]
   if (appbase_ops != pre_appbase_pattern):
      return False

   return True

COMPARATORS = { 'json_compare': isAppbaseOperations }
