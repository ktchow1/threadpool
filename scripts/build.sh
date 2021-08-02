#!/usr/bin/env bash

pushd "$(dirname "${BASH_SOURCE[0]}")/.." &> /dev/null

# docker run --rm -v "$(pwd):$(pwd)" -w "$(pwd)" ytl7.2:10.2.0 scripts/make.sh ${@}
scripts/make.sh ${@}
rc=${?}

popd &> /dev/null

exit ${rc}
