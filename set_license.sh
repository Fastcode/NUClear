#!/bin/bash

# Loop through all the files from git ls-files
for file in $(git ls-files); do
    # Get the year this file was added
    year=$(git log --follow --format=%aD ${file} | tail -1 | awk '{print $4}')
    echo ${file} ${year}
    licenseheaders \
        -t .licence.tmpl \
        --years="${year}" \
        --owner "NUClear Contributors" \
        --projname "NUClear" \
        --projurl="https://github.com/Fastcode/NUClear" \
        -f ${file}
done
