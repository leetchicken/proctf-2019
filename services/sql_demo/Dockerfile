FROM debian:bullseye

RUN apt-get update && apt-get install -y socat python

WORKDIR /app
COPY service.py ./

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password --disabled-login sql

RUN mkdir -p /data && chown -R sql /data
VOLUME /data

WORKDIR /data

USER sql

CMD socat -s TCP-LISTEN:1433,reuseaddr,fork EXEC:"/usr/bin/python /app/service.py"
