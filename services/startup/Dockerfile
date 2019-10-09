FROM debian:stable-slim

RUN apt-get update
COPY start.sh updater transport_http transport_http2 /var/www/

WORKDIR /var/www/
CMD ["./start.sh"]
