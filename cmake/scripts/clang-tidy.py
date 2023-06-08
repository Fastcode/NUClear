#! /usr/bin/env python3

import hashlib
import json
import os
import subprocess
import sys

# Get the relevant arguments for this script
output_folder = sys.argv[1]

# Remainder of the arguments are the arguments to pass to clang-tidy
ignore_file = sys.argv[2]
ct_cache = sys.argv[3]
ct_bin = sys.argv[4]
ct_args = sys.argv[5:]

# Make the fixes and cache folders
cache_folder = os.path.join(output_folder, "cache")
fixes_folder = os.path.join(output_folder, "fixes")
os.makedirs(fixes_folder, exist_ok=True)
os.makedirs(cache_folder, exist_ok=True)

# Hash the remaining arguments as the output file name for later application
output_file = os.path.join(
    fixes_folder,
    "{}.yaml".format(hashlib.sha256(" ".join(ct_args).encode("utf-8")).hexdigest()),
)

# Remove the output file if it exists
if os.path.exists(output_file):
    os.remove(output_file)

# Call clang tidy with the provided args
result = subprocess.run(
    [
        ct_cache,
        ct_bin,
        "--use-color",
        "--export-fixes={}".format(os.path.join(fixes_folder, output_file)),
        *ct_args,
    ],
    env={
        **os.environ,
        "CTCACHE_DIR": os.getenv("CTCACHE_DIR", cache_folder),
    },
)

# Pass the return code back to the caller
exit(result.returncode)
