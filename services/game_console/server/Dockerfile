FROM ubuntu:18.04
WORKDIR /home/server

RUN apt-get update && \
	apt-get install -y libpugixml1v5 libmicrohttpd12 unzip libcap2-bin

RUN addgroup --system --gid 10101 server
RUN adduser --system --disabled-password --uid 10101 --shell /sbin/nologin --no-create-home --home /home/server --gid 10101 server

COPY ./server .
COPY ./data/ ./data/
COPY ./entrypoint.sh .

RUN chown -R server:server /home/server

RUN setcap CAP_NET_BIND_SERVICE=+eip /home/server/server

CMD ["./entrypoint.sh"]
