
import jinja2

import argparse
import json
import os
import sys

def needs_overwrite( target_path, text ):
    if not os.path.exists( target_path ):
        return True
    with open(target_path, "r") as f:
        current_text = f.read()
    if current_text == text:
        return False
    return True

def overwrite_if_different( target_path, text ):
    if not needs_overwrite( target_path, text ):
        return
    with open(target_path, "w") as f:
        f.write(text)
    return

def build(template_dir="templates", output_dir=None, ctx=None, do_write=True, deps=None, outputs=None, jinja2_env=None):
    if jinja2_env is None:
        jinja2_env = jinja2.Environment()
        jinja2_env.filters["json_to_char_array"] = json_to_char_array
    os.makedirs(output_dir, exist_ok=True)
    for root, dirs, files in os.walk(template_dir):
        dirs_to_remove = [d for d in dirs if d.startswith(".")]
        for d in dirs_to_remove:
            dirs.remove(d)
        for f in files:
            fpath = os.path.join(root, f)
            rpath = os.path.relpath(fpath, template_dir)
            target_path = os.path.join(output_dir, rpath)
            target_dir = os.path.dirname(target_path)
            os.makedirs(target_dir, exist_ok=True)
            with open(fpath, "r") as infile:
                infile_text = infile.read()
            if f.endswith(".j2"):
                target_path = target_path[:-3]
            if do_write:
                if f.endswith(".j2"):
                    template = jinja2_env.from_string( infile_text )
                    outfile_text = template.render( **ctx )
                else:
                    outfile_text = infile_text
                overwrite_if_different( target_path, outfile_text )
            if deps is not None:
                deps.append(fpath)
            if outputs is not None:
                outputs.append(target_path)
    return

def load_context(json_dir, ctx=None, do_read=True, deps=None):
    if ctx is None:
        ctx = {}
    for root, dirs, files in os.walk(json_dir):
        dirs_to_remove = [d for d in dirs if d.startswith(".")]
        for d in dirs_to_remove:
            dirs.remove(d)
        dirs.sort()
        for f in sorted(files):
            if not f.endswith(".json"):
                continue
            fpath = os.path.join(root, f)
            if do_read:
                with open(fpath, "r") as infile:
                    obj = json.load(infile)
                ctx.update(obj)
            if deps is not None:
                deps.append(fpath)
    return ctx

def json_to_char_array(json_object):
    s = json.dumps(json_object, separators=(",", ":") )
    a = []
    n = len(s)
    j = 0
    for i in range(n):
        c = str(ord(s[i]))
        a.append(" "*(3-len(c)))
        a.append(c)
        if i < n-1:
            a.append(",")
        j += 1
        if j == 20:
            a.append("\n")
            j = 0
    return "".join(a)

def main(argv):

    parser = argparse.ArgumentParser( description="Build the manifest library" )
    parser.add_argument( "--json-dir", "-j", type=str, required=True, help="Location of JSON template context files")
    parser.add_argument( "--template-dir", "-t", type=str, required=True, help="Location of template files" )
    parser.add_argument( "--output-dir", "-o", type=str, required=True, help="Output location" )
    parser.add_argument( "--print-dependencies", action="store_true", help="Print dependencies and exit")
    parser.add_argument( "--print-outputs", action="store_true", help="Print dependencies and exit")
    args = parser.parse_args()

    deps = []
    outputs = []

    do_write = not (args.print_dependencies or args.print_outputs)

    ctx = load_context(args.json_dir, do_read=do_write, deps=deps)

    build(
        template_dir=args.template_dir,
        output_dir=args.output_dir,
        ctx=ctx,
        do_write=do_write,
        deps=deps,
        outputs=outputs,
        )

    if args.print_dependencies:
        print(";".join(deps))
    if args.print_outputs:
        print(";".join(outputs))

if __name__ == "__main__":
    main(sys.argv)
