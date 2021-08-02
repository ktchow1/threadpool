#!/usr/bin/env bash

if [[ ${#} -lt 1 ]]; then
  exit 1
fi

BUILD_NAME="${1}"
BUILD_DIR="build/${1}"
shift

pushd "$(dirname "${BASH_SOURCE[0]}")/.." &> /dev/null

mkdir -p "${BUILD_DIR}"
pushd "${BUILD_DIR}" &> /dev/null

ulimit -c unlimited
CMAKE="../../scripts/build/${BUILD_NAME}.sh"
cat "${CMAKE}" | grep cmake
"${CMAKE}"
rc=${?}
if [[ ${rc} -ne 0 ]]; then
  exit ${rc}
fi

# make -j ${nproc} ${@}
make -j4 ${@}
rc=${?}
if [[ ${rc} -ne 0 ]]; then
  exit ${rc}
fi

popd &> /dev/null
popd &> /dev/null
