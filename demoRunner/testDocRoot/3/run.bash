#!/bin/bash

function term()
{
    echo "Caught signal SIGINT"
    set -x
    kill -TERM $childPid
}

trap term SIGINT SIGTERM
xlogo &
childPid=$!
wait $childPid
