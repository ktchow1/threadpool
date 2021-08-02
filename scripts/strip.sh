#!/usr/bin/env bash

objcopy --only-keep-debug "${1}" "${1}.debug"
strip --strip-debug --strip-unneeded "${1}"
