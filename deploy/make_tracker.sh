#!/bin/bash

pushd ../services/tracker
    docker-compose -f docker-compose-build.yml build
popd

./make_service_ova.sh tracker
