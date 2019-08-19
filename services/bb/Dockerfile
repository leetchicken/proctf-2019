FROM perl:5.30

RUN cpanm -n Mojolicious Mojo::Pg IO::Socket::SSL

WORKDIR /app
COPY . /app

CMD ["/usr/local/bin/hypnotoad", "-f", "script/bb"]
