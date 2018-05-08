#!/usr/bin/env python3

import sys
from pathlib import Path

def print_usage(program_name):
    print("usage: {} DIR OUTFILE".format(program_name))

def generate_concatenated_outfile(input_dir, suffix_filter="hf"):
    def predicate(path):
        if not path.is_file():
            return false
        if suffix_filter == "":
            return true
        else:
            return path.suffix == ("." + suffix_filter)
    input_files = [p for p in input_dir.iterdir() if predicate(p)]
    output = ""
    for p in sorted(input_files):
        with p.open(mode='r') as f:
            output += f.read()
    return output

def main(program_name, args):
    if len(args) < 2:
        print_usage(program_name)
        return 1

    input_dir = Path(args[0])
    if not input_dir.is_dir():
        print_usage(program_name)
        return 1

    out_file = Path(args[1])
    if out_file.exists():
        if not out_file.is_file():
            print('Chosen OUTFILE "{}" is not a file'.format(out_file.absolute().as_posix()))
            return 1
        new_outfile_contents = generate_concatenated_outfile(input_dir)
        with out_file.open(mode='r') as f:
            if f.read() == new_outfile_contents:
                print('File "{}" up-to-date with .d directory'.format(out_file))
                return 0
    else:
        if not out_file.parent.exists():
            try:
                out_file.parent.mkdir(parents=True)
            except FileExistsError:
                pass
            except:
                print('Unexpected error occured while trying to create directory "{}"'.format(out_file.parent.absolute().as_posix()))
                raise
        elif not out_file.parent.is_dir():
            print('"{}" is not a directory'.format(out_file.parent.absolute().as_posix()))
            return 1
        new_outfile_contents = generate_concatenated_outfile(input_dir)

    with out_file.open(mode='w') as f:
        f.write(new_outfile_contents)
    print('Built "{}" from .d directory'.format(out_file))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[0], sys.argv[1:]))
