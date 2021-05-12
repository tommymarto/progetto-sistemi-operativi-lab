#!/bin/bash

# set defaults
N_THREAD_WORKERS=4
MAX_MEMORY_SIZE_MB=128
MAX_FILES=1000
SOCKET_FILEPATH=/tmp/fileStorageSocket.sk
CACHE_POLICY=FIFO
LOG_VERBOSITY=2

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --workers-count)
    N_THREAD_WORKERS="$2"
    shift # past argument
    shift # past value
    ;;
    --max-memory)
    MAX_MEMORY_SIZE_MB="$2"
    shift # past argument
    shift # past value
    ;;
    --max-files)
    MAX_FILES="$2"
    shift # past argument
    shift # past value
    ;;
    --socket-filepath)
    SOCKET_FILEPATH="$2"
    shift # past argument
    shift # past value
    ;;
    --cache-policy)
    CACHE_POLICY="$2"
    shift # past argument
    shift # past value
    ;;
    --log-verbosity)
    LOG_VERBOSITY="$2"
    shift # past argument
    shift # past value
    ;;
    # *)    # unknown option
    # POSITIONAL+=("$1") # save it in an array for later
    # shift # past argument
    # ;;
esac
done

export N_THREAD_WORKERS
export MAX_MEMORY_SIZE_MB
export MAX_FILES
export SOCKET_FILEPATH
export CACHE_POLICY
export LOG_VERBOSITY

envsubst < config_template.txt > config.txt