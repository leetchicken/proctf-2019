#!/bin/bash -e

pushd ../services/handy
make build
popd

./make_service_ova.sh handy