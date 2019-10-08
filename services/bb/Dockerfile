FROM perl:5.30-slim

RUN apt-get update && \
  apt-get install -y --no-install-recommends \
    libssl-dev libc6-dev zlib1g-dev openssl gcc libpq-dev

RUN cpanm -n Mojolicious Mojo::Pg IO::Socket::SSL

WORKDIR /app
COPY . /app

CMD ["/usr/local/bin/hypnotoad", "-f", "script/bb"]
