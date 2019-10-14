#!/bin/bash -e

pushd ../services/dotFM
./build.sh
popd

./make_service_ova.sh dotfm
