#!/bin/bash

CLIENT="./bin/client/bin/client"

if [ $# -eq 1 ]; then
    CLIENT="$1"
fi

LIST_OF_FILES=($( tree -f -F -i ./testFiles/test | grep -v /$ | tail -n +2 | head -n -2 ))

RUNNING=0
MIN_PROC=15

sleep 1

while :; do
    while (( $RUNNING <= $MIN_PROC )); do
        RANDOM_FILES=($(shuf -e -n 10 "${LIST_OF_FILES[@]}"))
        ADD_RANDOM_FILES=($( for i in ${RANDOM_FILES[@]}; do printf -- "-W ${i} "; done ))
        LOCK_RANDOM_FILES=($( for i in ${RANDOM_FILES[@]}; do printf -- "-l ${i} "; done ))
        $CLIENT -q \
                ${ADD_RANDOM_FILES[@]} \
                -D ./destDir \
                ${LOCK_RANDOM_FILES[@]} \
                -c ${RANDOM_FILES[0]} \
                -c ${RANDOM_FILES[1]} \
                -c ${RANDOM_FILES[2]} \
                -c ${RANDOM_FILES[3]} \
                -c ${RANDOM_FILES[4]} \
                -c ${RANDOM_FILES[5]} \
                -c ${RANDOM_FILES[6]} \
                -c ${RANDOM_FILES[7]} \
                -c ${RANDOM_FILES[8]} &
        RUNNING=$(( $RUNNING + 1 ))
    done
    wait -n
    RUNNING=$(( $RUNNING - 1 ))
done