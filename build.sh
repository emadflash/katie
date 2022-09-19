#!/usr/bin/env bash

set -eu

: ${CC=clang}
: ${CFLAGS=}
: ${LDFLAGS=}

TARGET="katie"
CFLAGS="$CFLAGS -std=c99"

panic() {
    printf "%s\n" "$1"
    exit 1
}

build_katie() {
    case $1 in
    debug)
        #EXTRAFLAGS="-Wall -Wextra -pedantic -ggdb -DDebug -fsanitize=address"
        EXTRAFLAGS="-Wall -Wextra -pedantic -ggdb -DDebug"
        ;;

    release)
        EXTRAFLAGS="-O3 -DRelease"
        ;;

    *)
        panic "Build mode unsupported!"
    esac

    set -x
    $CC $CFLAGS $EXTRAFLAGS $LDFLAGS main.c -o $TARGET
    set +x
}

if [[ $# -eq 0 ]]; then
    build_katie debug
    exit 0
fi

if [[ $# -eq 1 ]]; then
    build_katie $1
    exit 0
else
    panic "Too many arguments"
fi
