FROM ubuntu:19.04

RUN apt-get update
RUN apt-get install -y python3 python3-aiohttp
RUN useradd --uid 10000 -m polyfill

WORKDIR /home/polyfill/
USER polyfill
CMD ["./polyfill_http.py"]
