FROM ubuntu:19.04

RUN apt-get update
RUN apt-get install -y xinetd curl xz-utils

RUN useradd --uid 10000 -m polyfill

RUN su polyfill -c "curl https://wasmtime.dev/install.sh -sSf | bash"

COPY xinetd.conf /home/polyfill/

WORKDIR /home/polyfill/
CMD ["./start.sh"]
