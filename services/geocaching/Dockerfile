FROM alpine
WORKDIR /app

COPY ./start.sh .
COPY ./out/ /app/

RUN apk add --no-cache sqlite-libs

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password geocacher

RUN mkdir -p /app/data
VOLUME /app/data

CMD ["./start.sh"]
