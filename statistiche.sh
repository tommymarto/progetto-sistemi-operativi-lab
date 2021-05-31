#!/bin/bash

LOG_FILENAME="serverLog.txt"

if [ $# -eq 1 ]; then
    LOG_FILENAME="$1"
fi

# read statistics
N_READS=0
BYTES_READ=0;

# write statistics
N_WRITES=0
BYTES_WRITTEN=0

# operations statistics
N_LOCK=0
N_UNLOCK=0
N_OPEN=0
N_OPEN_LOCK=0
N_CLOSE=0

# filesystem statistics
MAX_FILESYSTEM_SIZE=0
MAX_FILESYSTEM_FILE_COUNT=0
CACHE_ACTIVATIONS=0

# worker statistics
WORKER_REQUESTS_HANDLED=()
NOTIFIER_REQUESTS_HANDLED=0

# connections statistics
MAX_SIMULTANEOUS_CONNECTIONS=0

# util to match newline
nl=$'\n'

# apro il file in lettura e gli assegno il descrittore 3 
exec 3<$LOG_FILENAME

# loop through file lines
while read -u 3 line; do
    if [[ $line =~ ("bytesRead: ")([0-9]+) ]]; then
    	N_READS=$(( N_READS + 1 ))
        BYTES_READ=$(( BYTES_READ + ${BASH_REMATCH[2]} ))
    
    elif [[ $line =~ ("bytesWritten: ")([0-9]+) ]]; then
    	N_WRITES=$(( N_WRITES + 1 ))
        BYTES_WRITTEN=$(( BYTES_WRITTEN + ${BASH_REMATCH[2]} ))
        
    elif [[ $line =~ ("requestServedBy: ")([0-9]+) ]]; then
        WORKER_REQUESTS_HANDLED[${BASH_REMATCH[2]}]=$((WORKER_REQUESTS_HANDLED[${BASH_REMATCH[2]}] + 1))
    
    elif [[ $line =~ "requestServedByNotifier" ]]; then
        NOTIFIER_REQUESTS_HANDLED=$(( NOTIFIER_REQUESTS_HANDLED + 1 ))
    
    elif [[ $line =~ ("openFile: ")(.*?)" flags: "([0-9]+) ]]; then
        # O_CREATE = 1<<0, O_LOCK = 1<<1
        if (( (${BASH_REMATCH[3]} & 2#10) != 0 )); then
    	    N_LOCK=$(( N_LOCK + 1 ))
    	    N_OPEN_LOCK=$(( N_OPEN_LOCK + 1 ))
        fi
        N_OPEN=$(( N_OPEN + 1 ))
    
    elif [[ $line =~ ("closeFile: ")([^$nl]*) ]]; then
        N_CLOSE=$(( N_CLOSE + 1 ))

    elif [[ $line =~ ("lockFile: ")([^$nl]*) ]]; then
    	N_LOCK=$(( N_LOCK + 1 ))
    
    elif [[ $line =~ ("unlockFile: ")([^$nl]*) ]]; then  
        # lockFile is a prefix of unlockFile so unlockFile must be checked first
    	N_UNLOCK=$(( N_UNLOCK + 1 ))

    elif [[ $line =~ ("maxFileSystemSize: ")([0-9]+) ]]; then
        MAX_FILESYSTEM_SIZE=$(( MAX_FILESYSTEM_SIZE > ${BASH_REMATCH[2]} ? MAX_FILESYSTEM_SIZE : ${BASH_REMATCH[2]} ))
    
    elif [[ $line =~ ("maxFileSystemFileCount: ")([0-9]+) ]]; then
        MAX_FILESYSTEM_FILE_COUNT=$(( MAX_FILESYSTEM_FILE_COUNT > ${BASH_REMATCH[2]} ? MAX_FILESYSTEM_FILE_COUNT : ${BASH_REMATCH[2]} ))
    
    elif [[ $line =~ ("cacheAlgorithmActivations: ")([0-9]+) ]]; then
        CACHE_ACTIVATIONS=$(( CACHE_ACTIVATIONS > ${BASH_REMATCH[2]} ? CACHE_ACTIVATIONS : ${BASH_REMATCH[2]} ))
    
    elif [[ $line =~ ("maxClientsConnected: ")([0-9]+) ]]; then
        MAX_SIMULTANEOUS_CONNECTIONS=$(( MAX_SIMULTANEOUS_CONNECTIONS > ${BASH_REMATCH[2]} ? MAX_SIMULTANEOUS_CONNECTIONS : ${BASH_REMATCH[2]} ))
    fi
done

# close descriptor
exec 3<&-

AVG_BYTES_READ=$(printf "%13.2f" 0)
AVG_BYTES_WRITTEN=$(printf "%13.2f" 0)

if (( N_READS != 0 )); then 
    AVG_BYTES_READ=$(echo "${BYTES_READ} ${N_READS}" | awk '{printf "%13.2f", $1/$2}')
fi
if (( N_WRITES != 0 )); then 
    AVG_BYTES_WRITTEN=$(echo "${BYTES_WRITTEN} ${N_WRITES}" | awk '{printf "%13.2f", $1/$2}')
fi

echo "╔══════════════════════════════════════════════════════╗"
echo "║                                                      ║"
echo "║             STATISTICS.SH - FINAL REPORT             ║"
echo "║                                                      ║"
echo "╟──────────────────────────────────────────────────────╢"
echo "║                                                      ║"
echo "║   TRANSMISSION STATISTICS                            ║"
echo "║                                                      ║"
echo "║   N_READS                      : $(printf "%10d" ${N_READS})          ║"
echo "║   TOTAL_BYTES_READ             : $(printf "%10d" ${BYTES_READ})     (B)  ║"
echo "║   TOTAL_BYTES_READ             : $(printf "%10d" $(( BYTES_READ / (1024*1024) )))    (MB)  ║"
echo "║   AVG_BYTES_READ               : ${AVG_BYTES_READ}  (B)  ║"
echo "║   N_WRITES                     : $(printf "%10d" ${N_WRITES})          ║"
echo "║   TOTAL_BYTES_WRITTEN          : $(printf "%10d" ${BYTES_WRITTEN})     (B)  ║"
echo "║   TOTAL_BYTES_WRITTEN          : $(printf "%10d" $(( BYTES_WRITTEN / (1024*1024) )))    (MB)  ║"
echo "║   AVG_BYTES_WRITTEN            : ${AVG_BYTES_WRITTEN}  (B)  ║"
echo "║                                                      ║"
echo "╟──────────────────────────────────────────────────────╢"
echo "║                                                      ║"
echo "║   OPERATION STATISTICS                               ║"
echo "║                                                      ║"
echo "║   N_LOCK                       : $(printf "%10d" ${N_LOCK})          ║"
echo "║   N_UNLOCK                     : $(printf "%10d" ${N_UNLOCK})          ║"
echo "║   N_OPEN                       : $(printf "%10d" ${N_OPEN})          ║"
echo "║   N_OPEN_LOCK                  : $(printf "%10d" ${N_OPEN_LOCK})          ║"
echo "║   N_CLOSE                      : $(printf "%10d" ${N_CLOSE})          ║"
echo "║                                                      ║"
echo "╟──────────────────────────────────────────────────────╢"
echo "║                                                      ║"
echo "║   FILESYSTEM STATISTICS                              ║"
echo "║                                                      ║"
echo "║   MAX_FILESYSTEM_SIZE          : $(printf "%10d" ${MAX_FILESYSTEM_SIZE})     (B)  ║"
echo "║   MAX_FILESYSTEM_SIZE          : $(printf "%10d" $(( MAX_FILESYSTEM_SIZE / (1024*1024) )))    (MB)  ║"
echo "║   MAX_FILESYSTEM_FILE_COUNT    : $(printf "%10d" ${MAX_FILESYSTEM_FILE_COUNT})          ║"
echo "║   CACHE_ACTIVATIONS            : $(printf "%10d" ${CACHE_ACTIVATIONS})          ║"
echo "║                                                      ║"
echo "╟──────────────────────────────────────────────────────╢"
echo "║                                                      ║"
echo "║   OTHER SERVER STATISTICS                            ║"
echo "║                                                      ║"

for i in ${!WORKER_REQUESTS_HANDLED[@]}; do
    echo "║   WORKER $(printf "%2d" $i) REQUESTS_HANDLED   : $(printf "%10d" ${WORKER_REQUESTS_HANDLED[$i]})          ║"
done

echo "║                                                      ║"
echo "║   NOTIFIER_REQUESTS_HANDLED    : $(printf "%10d" ${NOTIFIER_REQUESTS_HANDLED})          ║"
echo "║   MAX_SIMULTANEOUS_CONNECTIONS : $(printf "%10d" ${MAX_SIMULTANEOUS_CONNECTIONS})          ║"
echo "║                                                      ║"
echo "╚══════════════════════════════════════════════════════╝"