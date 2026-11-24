#!/bin/sh
set -eu

DIR=/shared
LOCK="$DIR/.lock"
ID=$(od -An -N8 -tx1 /dev/urandom | tr -d ' \n')
N=0
STOP=
FILE=

trap 'STOP=1' TERM INT
trap '[ -z "$FILE" ] || rm -f "$FILE"' EXIT

exec 9>"$LOCK"

while [ -z "$STOP" ]; do
    N=$((N + 1))

    flock 9
    i=1
    while :; do
        case $i in
            ?)  NAME=00$i ;;
            ??) NAME=0$i ;;
            *)  NAME=$i ;;
        esac
        [ -e "$DIR/$NAME" ] || break
        i=$((i + 1))
    done
    FILE="$DIR/$NAME"
    : > "$FILE"
    flock -u 9

    printf '%s %s\n' "$ID" "$N" > "$FILE"
    sleep 1
    rm -f "$FILE"
    FILE=
    [ -z "$STOP" ] || break
    sleep 1
done
