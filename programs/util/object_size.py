#!/usr/bin/env python3

import json

builtin_type_sizes = {
   "int8_t" : 1,
   "int16_t" : 2,
   "int32_t" : 4,
   "int64_t" : 8,
   "uint8_t" : 1,
   "uint16_t" : 2,
   "uint32_t" : 4,
   "uint64_t" : 8,

   "char" : 1,
   "bool" : 1,

   "fc::time_point_sec" : 4,
   "steem::protocol::asset_symbol_type" : 8,
   "steem::protocol::account_name_type" : 16,
}

class TermSum(object):
    def __init__(self):
        self.term_to_count = {}

    def __add__(self, rhs):
        if rhs == 0:
            return self
        if isinstance(rhs, int):
            self.term_to_count[""] += rhs
            return
        if not isinstance(rhs, TermSum):
            raise NotImplemented
        result = TermSum()
        result.term_to_count = dict(self.term_to_count)
        for term, count in rhs.term_to_count.items():
            result.term_to_count[term] = result.term_to_count.get(term, 0)+count
        return result

    def __radd__(self, lhs):
        return self.__add__(lhs)

    def __str__(self):
        return "+".join( "{} {}".format( count, term ) for term, count in sorted(self.term_to_count.items()) )

def term(count, name):
    ts = TermSum()
    ts.term_to_count = {name : count}
    return ts

class Sizer(object):
    def __init__(self, schema_map=None):
        self.type_to_size = {}
        self.schema_map = schema_map

    def get_size(self, name):
        y = self.type_to_size.get(name)
        if y is not None:
            return y
        y = self._compute_size( name )
        self.type_to_size[name] = y
        return y

    def _compute_size( self, name ):
        y = builtin_type_sizes.get(name)
        if y is not None:
            return term(y, "bytes")

        s = self.schema_map[name]
        deps = s["deps"]
        schema = s["schema"]
        t = schema["type"]

        if t == "oid":
            return term(8, "bytes")
        if t == "class":
            return sum( self.get_size(mtype) for mtype, mname in schema["members"] )
        return term(1, name)

def main():
    with open("steem.schema", "r") as f:
        schema = json.load(f)

    schema_map = schema["schema_map"]
    chain_object_types = schema["chain_object_types"]
    sizer = Sizer(schema_map=schema_map)
    for t in chain_object_types:
        s = sizer.get_size(t)
        print(t)
        print("    "+str(s))

if __name__ == "__main__":
    main()
