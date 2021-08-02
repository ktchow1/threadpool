#!/usr/bin/env bash

pushd "$(dirname "${BASH_SOURCE[0]}")/.." &> /dev/null

docker run --rm -it -v "$(pwd):$(pwd)" -w "$(pwd)" ytl7.2:10.2.0 bash

popd &> /dev/null
