FROM ubuntu:18.04 as build
WORKDIR /home/build

RUN apt-get update && apt-get install -y libmicrohttpd-dev libssl-dev antlr4 g++ make nasm

COPY . .

WORKDIR /home/build/compiler
RUN make -j8

WORKDIR /home/build
RUN make -j8

FROM ubuntu:18.04
WORKDIR /home/convolution

RUN apt-get update && apt-get install -y libmicrohttpd12 libssl1.1

COPY --from=build /home/build/convolution .
COPY ./start.sh .
COPY ./pubkey.pem .

RUN adduser --system --home /home/convolution --shell /usr/sbin/nologin --no-create-home --disabled-password convolution

RUN mkdir -p /home/convolution/data
VOLUME /home/convolution/data

CMD ["./start.sh"]
