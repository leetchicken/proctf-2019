#!/bin/bash

pushd frontend
# Build the docker container for building the frontend
docker build -f Dockerfile.build --tag drone_racing_frontend_build .
popd
# Build frontend!
docker run -v "$PWD/frontend":/home/frontend -v /home/frontend/node_modules drone_racing_frontend_build

# Build the docker container for building the application
docker build -f backend/Dockerfile.build --tag drone_racing_backend_build .
# Build our application!
docker run -v "$PWD/backend":/home/build drone_racing_backend_build 
