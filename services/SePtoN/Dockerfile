FROM swift:latest as build
WORKDIR /home/SePtoN/

COPY ./Sources ./Sources
COPY ./Package.swift ./Package.swift

RUN swift build -c release


FROM swift:slim
WORKDIR /home/SePtoN/

COPY --from=build /home/SePtoN/.build/x86_64-unknown-linux/release/SePtoN .
COPY ./start.sh .

RUN adduser --system --home /home/SePtoN --shell /usr/sbin/nologin --no-create-home --disabled-password SePtoN

RUN mkdir -p /home/SePtoN/data
VOLUME /home/SePtoN/data

CMD ["./start.sh"]
