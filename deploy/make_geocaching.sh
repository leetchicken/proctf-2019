#!/bin/bash -e

pushd ../services/geocaching
./build.sh
popd

./make_service_ova.sh geocaching
