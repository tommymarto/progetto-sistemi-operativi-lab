#!/bin/bash

CLIENT="./bin/client/bin/client"

CUR_PWD=$(pwd)

if [ $# -eq 1 ]; then
    CLIENT="$1"
fi

# sequential test
$CLIENT -t 200 -p \
        -w ./testFiles/otherRandomGeneratedFiles \
        -r ./testFiles/otherRandomGeneratedFiles/file1.txt \
        -W ${CUR_PWD}/testFiles/file2.txt \
        -r ${CUR_PWD}/testFiles/file2.txt \
        -c ${CUR_PWD}/testFiles/file2.txt \
        -R 3 -d ./destDir \
        -l ./testFiles/otherRandomGeneratedFiles/innerFolder/file1.txt \
        -l ./testFiles/otherRandomGeneratedFiles/innerFolder/file2.txt \
        -u ./testFiles/otherRandomGeneratedFiles/innerFolder/file2.txt
