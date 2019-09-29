FROM perl:5.30

RUN cpanm -n \
  Mojolicious Cpanel::JSON::XS Crypt::JWT IO::Socket::SSL \
  IPC::Run String::Random Mojo::SQLite Data::UUID::MT

RUN cpanm -n DDP

WORKDIR /app
COPY . /app

# morbo -l 'https://*:8443?cert=./ssl/cert.pem&key=./ssl/key.pem' ./app.pl
# CMD ["hypnotoad", "-f", "app.pl"]
CMD ["/bin/bash"]
