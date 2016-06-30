#!/bin/bash

function term()
{
    echo "Caught signal SIGINT"
    set -x
    kill -TERM $childPid
}

trap term SIGINT SIGTERM
xlogo -geometry 272x163-0+88 -bg yellow -fg black &
childPid=$!
wait $childPid
