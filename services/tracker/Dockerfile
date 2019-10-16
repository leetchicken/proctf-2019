FROM python:3.7.3-alpine3.9 as build

WORKDIR /build
COPY . ./
RUN ./merge_and_compile.sh


FROM python:3.7.3-alpine3.9 as runtime

RUN apk add --update alpine-sdk

RUN adduser tracker -u 10000 -D
WORKDIR /home/tracker

COPY requirements.txt .
RUN pip3 install -r requirements.txt

COPY db ./db
COPY --from=build /build/tracker.bin .
COPY execute.py start.sh ./

CMD ["./start.sh"]
