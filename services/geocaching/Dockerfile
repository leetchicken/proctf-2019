FROM ubuntu:19.04
WORKDIR /app

COPY ./start.sh .
COPY ./out/ /app/

RUN apt update
RUN apt install -y sqlite3 libsqlite3-dev openssl libssl-dev

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password geocacher

RUN mkdir -p /app/data
VOLUME /app/data

CMD ["./start.sh"]
