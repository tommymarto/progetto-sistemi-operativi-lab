#!/bin/bash

CLIENT="./bin/client/bin/client"

if [ $# -eq 1 ]; then
    CLIENT="$1"
fi

echo "sequential test"
$CLIENT -p \
        -w ./testFiles/test/test2folder -D ./destDir \
echo "end sequential test"

printf "\n\n\n\n"
echo "concurrent test"
sleep 2

for i in {1..4}; do
    $CLIENT -q -p \
            -w ./testFiles/test/test2folder2/client$i -D ./destDir2 &
    pids[${i}]=$!
done

wait ${pids[@]}
