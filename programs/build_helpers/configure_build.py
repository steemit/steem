#!/usr/bin/env python3

import os
import sys
import subprocess
from pathlib import Path
import argparse
import re

class MyParser(argparse.ArgumentParser):
    # This is a hack to get help and usage of this script to show up properly
    def find_last_line_of_usage(self):
        help = super(MyParser, self).format_usage()
        helplines = help.splitlines()
        self._last_line_of_usage = len(helplines) - 1

    def format_help(self):
        help = super(MyParser, self).format_help()
        helplines = help.splitlines()
        helplines[self._last_line_of_usage] += " [-- [CMAKEOPTS ...]]"
        helplines.append("  CMAKEOPTS \t\tadditional options to pass directly to cmake")
        helplines.append("")    # Just a trick to force a linesep at the end
        return os.linesep.join(helplines)

    def format_usage(self):
        help = super(MyParser, self).format_usage()
        helplines = help.splitlines()
        helplines[self._last_line_of_usage] += " [-- [CMAKEOPTS ...]]"
        helplines.append("")    # Just a trick to force a linesep at the end
        return os.linesep.join(helplines)

def parse_arguments(src_default):
    def convert_to_path(path):
        p = Path(path)
        if p.exists():
            return p.resolve()
        else:
            return None

    def convert_to_dir(path):
        p = convert_to_path(path)
        if p and p.is_dir():
            return p
        else:
            return None

    def try_default_dir_from_env(args, flag_name, metavar_name, env_name=None):
        if not env_name:
            env_name = metavar_name
        if hasattr(args, flag_name):
            if not getattr(args, flag_name):
                parser.error("{} must be a valid directory".format(metavar_name))
        else:
            directory = os.environ.get(env_name)
            if directory:
                directory = convert_to_dir(directory)
                if not directory:
                    parser.error("{} environment variable must be a valid directory (or alternatively specify {} on command line)".format(env_name, metavar_name))
                else:
                    setattr(args, flag_name, directory)

    parser = MyParser(description="Helper function to call cmake with appropriate configuration to build Steem.")
    parser.add_argument("--sys-root", metavar="SYS_ROOT", type=convert_to_dir, default=argparse.SUPPRESS, 
                        help="Root directory to search within for libraries and header files (can alternatively specify with SYS_ROOT environment variable)")
    parser.add_argument("--boost-dir", metavar="BOOST_ROOT", type=convert_to_dir, default=argparse.SUPPRESS, 
                        help="Boost root directory (can alternatively specify with BOOST_ROOT environment variable)")
    parser.add_argument("--openssl-dir", metavar="OPENSSL_ROOT", type=convert_to_dir, default=argparse.SUPPRESS, 
                        help="OpenSSL root directory (can alternatively specify with OPENSSL_ROOT_DIR environment variable)")
    node_type = parser.add_mutually_exclusive_group()
    node_type.add_argument("-f", "--full", dest="low_mem_node", action="store_false", default=argparse.SUPPRESS, help="build with LOW_MEMORY_NODE=OFF (default)")
    node_type.add_argument("-w", "--witness", dest="low_mem_node", action="store_true", default=argparse.SUPPRESS, help="build with LOW_MEMORY_NODE=ON")
    build_type = parser.add_mutually_exclusive_group()
    build_type.add_argument("-r", "--release", dest="release", action="store_true", default=argparse.SUPPRESS, help="build with CMAKE_BUILD_TYPE=RELEASE (default)")
    build_type.add_argument("-d", "--debug", dest="release", action="store_false", default=argparse.SUPPRESS, help="built with CMAKE_BUILD_TYPE=DEBUG")
    parser.add_argument("--win", "--windows", dest="windows", action="store_true", default=argparse.SUPPRESS, help="cross compile for Windows using MinGW")
    parser.add_argument("--src", dest="source_dir", metavar="SOURCEDIR", type=convert_to_dir, default=argparse.SUPPRESS, 
                        help="Steem source directory (if omitted, will assume is at ../.. relative to location of this script)")
    parser.add_argument("additional_args", metavar="CMAKEOPTS", nargs=argparse.REMAINDER, help=argparse.SUPPRESS)

    parser.set_defaults(low_mem_node=False, release=True, windows=False, source_dir=src_default)

    parser.find_last_line_of_usage()
    args = parser.parse_args()
    if len(args.additional_args) > 0 and not args.additional_args[0] == "--":
        parser.error("CMAKEOPTS must come after --")
    args.additional_args = args.additional_args[1:]

    if not args.source_dir:
        parser.error("SOURCEDIR must be a valid directory")

    try_default_dir_from_env(args, "sys_root", "SYS_ROOT")
    try_default_dir_from_env(args, "boost_dir", "BOOST_ROOT")
    try_default_dir_from_env(args, "openssl_dir", "OPENSSL_ROOT", "OPENSSL_ROOT_DIR")

    return args

def find_boost_version(boost_dir):
    boost_version_header = boost_dir.joinpath("include/boost/version.hpp")
    if not boost_version_header.is_file():
        raise Exception("Boost version.hpp could not be found")
    with boost_version_header.open(mode="r") as f:
        boost_version_header_contents = f.read()
    m = re.search('^#define BOOST_VERSION (?P<version>\d+)$', boost_version_header_contents, flags=re.MULTILINE)
    try:
        boost_version = int(m.group('version'))
    except:
        raise Exception("Could not parse Boost version.hpp for version number")
    version = {}
    version["patch"] = boost_version % 100
    boost_version //= 100
    version["minor"] = boost_version % 1000
    boost_version //= 1000
    version["major"] = boost_version
    return version

def main(args):
    command = ["cmake"]
    root_search_paths = []

    if hasattr(args, "sys_root"):
        root_search_paths.append(args.sys_root.absolute().as_posix())

    # Maybe add Boost flags
    if hasattr(args, "boost_dir"):
        version = find_boost_version(args.boost_dir)
        boost_dir_path = args.boost_dir.absolute().as_posix()
        root_search_paths.append(boost_dir_path)
        command.append("-DBoost_NO_SYSTEM_PATHS=ON")
        command.append('-DBOOST_ROOT={}'.format(boost_dir_path))
        command.append('-DBoost_ADDITIONAL_VERSIONS="{major}.{minor}.{patch};{major}.{minor}"'
                       .format(major=version["major"], minor=version["minor"], patch=version["patch"]))

    # Maybe add OpenSSL flags
    if hasattr(args, "openssl_dir"):
        openssl_dir_path = args.openssl_dir.absolute().as_posix()
        root_search_paths.append(openssl_dir_path)
        command.append('-DOPENSSL_ROOT_DIR={}'.format(openssl_dir_path))

    # Maybe add Windows cross-compilation flags
    if args.windows:
        mingw = "x86_64-w64-mingw32"
        command.append("-DFULL_STATIC_BUILD=ON")
        command.append("-DCMAKE_SYSTEM_NAME=Windows")
        command.append("-DCMAKE_C_COMPILER={}-gcc".format(mingw))
        command.append("-DCMAKE_CXX_COMPILER={}-g++".format(mingw))
        command.append("-DCMAKE_RC_COMPILER={}-windres".format(mingw))
        command.append("-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER")
        command.append("-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY")
        command.append("-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY")

    # Add Steem flags
    command.append("-DLOW_MEMORY_NODE=" + ("ON" if args.low_mem_node else "OFF"))
    command.append("-DCMAKE_BUILD_TYPE=" + ("RELEASE" if args.release else "DEBUG"))

    # Add source directory, root paths to search within, and any additional flags/options passed to this script
    if len(root_search_paths) > 0:
       command.append("-DCMAKE_FIND_ROOT_PATH={}".format(";".join(root_search_paths)))
    command.append('{}'.format(args.source_dir.absolute().as_posix()))
    command = command + args.additional_args

    # Execute cmake
    print(" ".join(command))
    return subprocess.call(command)


if __name__ == "__main__":
    script_path = Path(os.path.realpath(__file__))
    args = parse_arguments(script_path.parent.parent.parent)
    sys.exit(main(args))
