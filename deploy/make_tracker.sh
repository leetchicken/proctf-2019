#!/bin/bash
set -e

pushd ../services/tracker
    ./build.sh
popd

./make_service_ova.sh tracker
