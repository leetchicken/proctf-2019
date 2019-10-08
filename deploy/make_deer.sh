#!/bin/bash -e

pushd ../services/deer
docker-compose build -f docker-compose-dev.yml 
popd

./make_service_ova.sh deer
