#!/bin/bash

# set defaults
N_THREAD_WORKERS=4
MAX_MEMORY_SIZE_MB=128
MAX_FILES=1000
MAX_FILE_SIZE=$((1<<24))
SOCKET_FILEPATH=fileStorageSocket.sk
MAX_CLIENTS=100
CACHE_POLICY=FIFO
LOG_VERBOSITY=4

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
    --max-file-size)
    MAX_FILE_SIZE="$2"
    shift # past argument
    shift # past value
    ;;
    --socket-filepath)
    SOCKET_FILEPATH="$2"
    shift # past argument
    shift # past value
    ;;
    --max-clients)
    MAX_CLIENTS="$2"
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
export MAX_FILE_SIZE
export SOCKET_FILEPATH
export MAX_CLIENTS
export CACHE_POLICY
export LOG_VERBOSITY

envsubst < config_template.txt > config.txt