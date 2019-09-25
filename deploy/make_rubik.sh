#!/bin/bash -e

pushd ../services/rubik
./build.sh
popd

./make_service_ova.sh rubik
