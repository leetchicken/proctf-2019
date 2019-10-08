#!/bin/bash -e

pushd ../services/deer
docker-compose -f docker-compose-dev.yml build 
popd

./make_service_ova.sh deer
