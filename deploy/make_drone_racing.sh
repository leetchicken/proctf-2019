#!/bin/bash -e

pushd ../services/drone_racing
./build.sh
popd

./make_service_ova.sh drone_racing