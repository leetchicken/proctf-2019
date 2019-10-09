#!/bin/bash -e

pushd ../services/notepool
./build.sh
popd

./make_service_ova.sh notepool
