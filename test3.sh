#!/bin/bash

CLIENT="./bin/client/bin/client"

if (( $# >= 1 )); then
    CLIENT="$1"
fi

LIST_OF_FILES=($( tree -f -F -i ./testFiles/test | grep -v /$ | tail -n +2 | head -n -2 ))

RUNNING=0
MIN_PROC=16

sleep 1

elapsed=0
start_time="$(date -u +%s)"
while (( elapsed <= 30 )); do
    while (( $RUNNING < $MIN_PROC )); do
        RANDOM_FILES=($(shuf -e -n 20 "${LIST_OF_FILES[@]}"))
        ADD_RANDOM_FILES=($( for i in ${RANDOM_FILES[@]}; do printf -- " -W ${i} "; done ))
        READ_RANDOM_FILES=($( for i in {0..10}; do printf -- " -r ${LIST_OF_FILES[$i]} "; done ))
        REMOVE_RANDOM_FILES=($( for i in {0..10}; do printf -- " -c ${LIST_OF_FILES[$i]} "; done ))
        # echo "$CLIENT -p ${ADD_RANDOM_FILES[@]} -D ./destDir ${READ_FILES} -c ${RANDOM_FILES[0]} "
        $CLIENT -q \
                ${ADD_RANDOM_FILES[@]} \
                -D ./destDir \
                ${READ_RANDOM_FILES[@]} \
                ${REMOVE_RANDOM_FILES[@]} &
        RUNNING=$(( $RUNNING + 1 ))
    done
    wait -n
    end_time="$(date -u +%s)"
    elapsed=$((end_time - start_time))

    RUNNING=$(( $RUNNING - 1 ))
done