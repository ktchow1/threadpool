#!/usr/bin/env bash

cp "${1}" "${1}.core"
objcopy --add-gnu-debuglink "${1}.debug" "${1}.core"
