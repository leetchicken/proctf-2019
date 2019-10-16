FROM perl:5.30-slim

RUN apt-get update && \
  apt-get install -y --no-install-recommends \
    libssl-dev libc6-dev zlib1g-dev netcat openssl gcc sqlite3

RUN cpanm -n \
  Mojolicious Cpanel::JSON::XS Crypt::JWT IO::Socket::SSL \
  String::Random Mojo::SQLite Data::UUID::MT

WORKDIR /app
COPY . /app

CMD ["morbo", "-l", "https://*:8443?cert=./ssl/cert.pem&key=./ssl/key.pem&ca=./ssl/cert.pem&verify=0x01", "./app.pl"]
