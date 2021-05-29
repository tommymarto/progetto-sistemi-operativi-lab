#!/bin/bash

CLIENT="./bin/client/bin/client"

CUR_PWD=$(pwd)

if [ $# -eq 1 ]; then
    CLIENT="$1"
fi

echo "sequential test"
$CLIENT -t 200 -p \
        -w ./testFiles/test/otherRandomGeneratedFiles \
        -r ./testFiles/test/otherRandomGeneratedFiles/file1.txt \
        -W ${CUR_PWD}/testFiles/test/file2.txt \
        -r ${CUR_PWD}/testFiles/test/file2.txt \
        -c ${CUR_PWD}/testFiles/test/file2.txt \
        -R 3 -d ./destDir \
        -l ./testFiles/test/otherRandomGeneratedFiles/innerFolder/file1.txt \
        -l ./testFiles/test/otherRandomGeneratedFiles/innerFolder/file2.txt \
        -u ./testFiles/test/otherRandomGeneratedFiles/innerFolder/file2.txt
echo "end sequential test"

printf "\n\n\n\n"
echo "concurrent locks test"
sleep 2

$CLIENT -p \
        -W ${CUR_PWD}/testFiles/test/file2.txt \

for i in {1..5}; do
    $CLIENT -t 1000 -p -q \
        -l ${CUR_PWD}/testFiles/test/file2.txt &
    pids[${i}]=$!
done

wait ${pids[@]}
