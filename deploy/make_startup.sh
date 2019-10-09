#!/bin/bash -e

pushd ../services/startup/src
source ~/.cargo/env
cargo build --release
rsync target/release/{transport_http2,updater,transport_http} ../
popd

./make_service_ova.sh startup
