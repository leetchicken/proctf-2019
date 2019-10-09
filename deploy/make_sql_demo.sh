#!/bin/bash -e

pushd ../services/sql_demo
docker-compose -f docker-compose-dev.yml build 
popd

./make_service_ova.sh sql_demo
